#pragma once

#include <atomic>
#include <memory>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

struct HashConfig {
  int replicas; // 初始每个节点的虚拟节点数
  int min_replicas;
  int max_replicas;
  double balance_threshold; // 负载不均阈值
};

class ConsistentHash {
public:
  ConsistentHash(HashConfig cfg = {10, 5, 200, 0.25});

  ~ConsistentHash();

  void AddNodes(const std::vector<std::string> &nodes);
  void RemoveNode(const std::string &node);

  std::string GetTargetNode(const std::string &key);
  void UpdateNodes(const std::vector<std::string> &nodes);
  // 获取统计信息,每个节点的请求/总请请求
  std::unordered_map<std::string, double> GetStats();

private:
  //CRC32 hash
  uint32_t hash_func(const std::string &data);

  void add_node_internal(const std::string &node, int replicas);
  void start_balancer();
  void check_and_rebalance();
  void rebalance_logic();
private:
  HashConfig config_;

  std::vector<uint32_t> keys_; // 排序后的k，用于二分查找最近的节点
  std::unordered_map<uint32_t, std::string> ring_; // key到地址的映射
  /*
    上面两个成员可优化为 std::vector<pair<uint32,string>>
    一次二分直接拿到地址，不用多一次map查找
  */

  std::unordered_map<std::string, int>
      node_replicas_;                     // 真实节点-》虚拟节点数量
  std::atomic<long long> total_requests_; // 全局请求总数
  std::unordered_map<std::string, std::shared_ptr<std::atomic<long long>>>
      node_counts_; // 每个节点的请求,用shared_ptr防止节点被删除导致balancer线程读到未定义值

  mutable std::shared_timed_mutex rw_mtx_;
  std::thread balancer_thread_;
  std::atomic<bool> stop_balancer_;
};