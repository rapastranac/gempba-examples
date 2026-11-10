#include <gempba/gempba.hpp>

#include <atomic>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <tuple>

/*
 * Stress test for a multithreaded run: one process, a pool of threads.
 *
 * Walks a full binary tree. A task is one tree node, (depth, index). Its children are (depth + 1, 2 * index) and
 * (depth + 1, 2 * index + 1), so the leaves are numbered 0 to 2^depth - 1. Each task hands its left child to a free
 * thread, when there is one, and visits its right child itself.
 *
 * However the threads split the work, two answers never change, so the test checks both:
 *  - the leaves visited are 2^depth: a lost or a repeated task changes the count;
 *  - the best result is the last leaf, 2^depth - 1.
 * A run that hangs never answers, and the test's timeout fails it.
 *
 * Usage: multithreading_leaf_visitor <threads> <depth>
 */

namespace {

    class leaf_visitor {
    public:
        leaf_visitor(gempba::load_balancer& p_load_balancer, gempba::node_manager& p_node_manager, const int p_depth) :
            m_load_balancer(p_load_balancer), m_node_manager(p_node_manager), m_depth(p_depth) {}

        void visit(const int p_depth, const int p_index, const gempba::node& p_parent) {
            if (p_depth == m_depth) {
                ++m_leaves;
                int v_leaf = p_index;
                m_node_manager.try_update_result(v_leaf, gempba::score::make(v_leaf));
                return;
            }
            gempba::node v_parent = p_parent == nullptr ? gempba::create_dummy_node(m_load_balancer) : p_parent;
            gempba::node v_left = gempba::create_explicit_node<void>(m_load_balancer, v_parent, m_visit, std::make_tuple(p_depth + 1, 2 * p_index));
            gempba::node v_right = gempba::create_explicit_node<void>(m_load_balancer, v_parent, m_visit, std::make_tuple(p_depth + 1, 2 * p_index + 1));
            m_node_manager.try_local_submit(v_left);
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
    };

} // namespace

int main(const int p_argc, char* p_argv[]) {
    if (p_argc != 3) {
        std::cerr << "usage: multithreading_leaf_visitor <threads> <depth>\n";
        return 2;
    }
    const int v_threads = std::stoi(p_argv[1]);
    const int v_depth = std::stoi(p_argv[2]);

    gempba::load_balancer* v_load_balancer = gempba::create_load_balancer(gempba::balancing_policy::QUASI_HORIZONTAL);
    gempba::node_manager& v_node_manager = gempba::create_node_manager(v_load_balancer);
    v_node_manager.set_goal(gempba::MAXIMISE, gempba::score_type::I32);
    v_node_manager.set_score(gempba::score::make(-1));
    v_node_manager.set_thread_pool_size(v_threads);

    leaf_visitor v_visitor(*v_load_balancer, v_node_manager, v_depth);
    gempba::node v_seed = gempba::create_seed_node<void>(*v_load_balancer, v_visitor.function(), std::make_tuple(0, 0));
    if (!v_node_manager.try_local_submit(v_seed)) {
        std::cerr << "unable to submit the seed\n";
        return 1;
    }
    v_node_manager.wait();

    const long long v_leaves = v_visitor.leaves();
    const int v_best_leaf = v_node_manager.get_result<int>().value_or(-1);
    const long long v_expected_leaves = 1LL << v_depth;
    const int v_expected_best_leaf = static_cast<int>(v_expected_leaves - 1);
    std::cout << v_threads << " threads: " << v_leaves << " leaves (expected " << v_expected_leaves << "), best leaf " << v_best_leaf << " (expected " << v_expected_best_leaf << ")\n";

    const int v_shutdown_code = gempba::shutdown();
    if (v_leaves != v_expected_leaves || v_best_leaf != v_expected_best_leaf) {
        return 1;
    }
    return v_shutdown_code;
}
