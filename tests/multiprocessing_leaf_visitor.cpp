#include <gempba/gempba.hpp>
#include <gempba/mpi.hpp>
#include <mpi.h>

#include <atomic>
#include <cstring>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <tuple>

/*
 * Stress test for a hybrid run: MPI ranks, each with a pool of threads, on the semi-centralized scheduler.
 *
 * Walks a full binary tree. A task is one tree node, (depth, index). Its children are (depth + 1, 2 * index) and
 * (depth + 1, 2 * index + 1), so the leaves are numbered 0 to 2^depth - 1. Each task hands its left child to another
 * rank, or to a free thread of its own when no rank is free, and visits its right child itself.
 *
 * However the ranks and threads split the work, two answers never change, so rank 0 checks both:
 *  - the leaves visited, summed over all ranks, are 2^depth: a lost or a repeated task changes the count;
 *  - the best result, which reaches rank 0 through gempba, is the last leaf, 2^depth - 1.
 * A run that hangs never answers, and the test's timeout fails it.
 *
 * Usage: mpiexec -n <ranks> multiprocessing_leaf_visitor <threads per rank> <depth>
 */

namespace {

    gempba::task_packet serialize(const int p_depth, const int p_index) {
        const int v_values[2] = {p_depth, p_index};
        return {reinterpret_cast<const char*>(v_values), sizeof(v_values)};
    }

    std::tuple<int, int> deserialize(const gempba::task_packet& p_packet) {
        int v_values[2];
        std::memcpy(v_values, p_packet.data(), sizeof(v_values));
        return {v_values[0], v_values[1]};
    }

    gempba::task_packet serialize_leaf(const int& p_leaf) { return {reinterpret_cast<const char*>(&p_leaf), sizeof(p_leaf)}; }

    class leaf_visitor {
    public:
        leaf_visitor(gempba::load_balancer& p_load_balancer, gempba::node_manager& p_node_manager, const int p_depth) :
            m_load_balancer(p_load_balancer), m_node_manager(p_node_manager), m_depth(p_depth) {}

        void visit(const int p_depth, const int p_index, const gempba::node& p_parent) {
            if (p_depth == m_depth) {
                ++m_leaves;
                int v_leaf = p_index;
                m_node_manager.try_update_result(v_leaf, gempba::score::make(v_leaf), m_leaf_serializer);
                return;
            }
            gempba::node v_parent = p_parent == nullptr ? gempba::create_dummy_node(m_load_balancer) : p_parent;
            gempba::node v_left = gempba::create_explicit_node<void>(m_load_balancer, v_parent, m_visit, std::make_tuple(p_depth + 1, 2 * p_index), m_serializer, m_deserializer);
            gempba::node v_right = gempba::create_explicit_node<void>(m_load_balancer, v_parent, m_visit, std::make_tuple(p_depth + 1, 2 * p_index + 1), m_serializer, m_deserializer);
            m_node_manager.try_remote_submit(v_left, 0);
            m_node_manager.forward(v_right);
        }

        [[nodiscard]] const std::function<void(std::thread::id, int, int, gempba::node)>& function() const { return m_visit; }

        [[nodiscard]] long long leaves() const { return m_leaves.load(); }

    private:
        gempba::load_balancer& m_load_balancer;
        gempba::node_manager& m_node_manager;
        const int m_depth;
        std::atomic<long long> m_leaves{0};
        std::function<void(std::thread::id, int, int, gempba::node)> m_visit = [this](std::thread::id, const int p_depth, const int p_index, const gempba::node& p_parent) {
            visit(p_depth, p_index, p_parent);
        };
        std::function<gempba::task_packet(int, int)> m_serializer = serialize;
        std::function<std::tuple<int, int>(gempba::task_packet)> m_deserializer = deserialize;
        std::function<gempba::task_packet(int&)> m_leaf_serializer = serialize_leaf;
    };

} // namespace

int main(const int p_argc, char* p_argv[]) {
    if (p_argc != 3) {
        std::cerr << "usage: mpiexec -n <ranks> multiprocessing_leaf_visitor <threads per rank> <depth>\n";
        return 2;
    }
    const int v_threads = std::stoi(p_argv[1]);
    const int v_depth = std::stoi(p_argv[2]);

    gempba::scheduler* v_scheduler = gempba::mpi::create_scheduler(gempba::mpi::scheduler_topology::SEMI_CENTRALIZED);
    gempba::load_balancer* v_load_balancer = gempba::create_load_balancer(gempba::balancing_policy::QUASI_HORIZONTAL, v_scheduler);
    gempba::node_manager& v_node_manager = gempba::create_node_manager(v_load_balancer, v_scheduler);
    v_scheduler->set_goal(gempba::MAXIMISE, gempba::score_type::I32);
    v_node_manager.set_goal(gempba::MAXIMISE, gempba::score_type::I32);
    v_node_manager.set_score(gempba::score::make(-1));
    if (!v_scheduler->is_root()) {
        v_node_manager.set_thread_pool_size(v_threads);
    }

    leaf_visitor v_visitor(*v_load_balancer, v_node_manager, v_depth);
    const std::function<std::tuple<int, int>(const gempba::task_packet&&)> v_args_deserializer = [](const gempba::task_packet&& p_packet) { return deserialize(p_packet); };
    const auto v_runnable = gempba::runnables::return_none::create(0, v_visitor.function(), v_args_deserializer);

    std::optional<gempba::task_bundle> v_seed;
    if (v_scheduler->is_root()) {
        v_seed.emplace(serialize(0, 0), v_runnable->get_id());
    }
    v_scheduler->run(v_node_manager, {{v_runnable->get_id(), v_runnable}}, std::move(v_seed));

    const long long v_my_leaves = v_visitor.leaves();
    long long v_leaves = 0;
    MPI_Reduce(&v_my_leaves, &v_leaves, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    int v_exit_code = 0;
    if (v_scheduler->is_root()) {
        const gempba::task_packet v_result = v_scheduler->get_result();
        int v_best_leaf = -1;
        if (v_result.size() == sizeof(v_best_leaf)) {
            std::memcpy(&v_best_leaf, v_result.data(), sizeof(v_best_leaf));
        }
        const long long v_expected_leaves = 1LL << v_depth;
        const int v_expected_best_leaf = static_cast<int>(v_expected_leaves - 1);
        std::cout << v_scheduler->world_size() << " ranks, " << v_threads << " threads per rank: " << v_leaves << " leaves (expected " << v_expected_leaves << "), best leaf " << v_best_leaf
                  << " (expected " << v_expected_best_leaf << ")\n";
        if (v_leaves != v_expected_leaves || v_best_leaf != v_expected_best_leaf) {
            v_exit_code = 1;
        }
    }

    const int v_shutdown_code = gempba::shutdown();
    return v_exit_code != 0 ? v_exit_code : v_shutdown_code;
}
