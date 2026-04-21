#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <omp.h>
#include <string>
#include <vector>

using json = nlohmann::json;

struct Point {
  int id;
  int x;
  int y;
  char type; // 't' - terminal, 's' - steiner
};

struct Edge {
  int id;
  int u;
  int v;
};

#define NUM_THREADS 8

int manhattan(const Point &a, const Point &b) {
  return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

int find_point_by_id(const std::vector<Point> &points, int id) {
  for (size_t i = 0; i < points.size(); ++i)
    if (points[i].id == id)
      return static_cast<int>(i);
  return -1;
}

double prim_mst(const std::vector<Point> &points, std::vector<int> &parent) {
  const int n = static_cast<int>(points.size());
  std::vector<bool> visited(n, false);
  std::vector<int> min_dist(n, INT_MAX);

  visited[0] = true;
  for (int i = 1; i < n; ++i) {
    min_dist[i] = manhattan(points[0], points[i]);
    parent[i] = 0;
  }

  double total = 0.0;
  for (int k = 1; k < n; ++k) {
    int u = -1;
    int best = INT_MAX;
    for (int i = 0; i < n; ++i) {
      if (!visited[i] && min_dist[i] < best) {
        best = min_dist[i];
        u = i;
      }
    }
    if (u == -1)
      break; // disconnected graph

    visited[u] = true;
    total += best;

    for (int v = 0; v < n; ++v) {
      if (!visited[v]) {
        int d = manhattan(points[u], points[v]);
        if (d < min_dist[v]) {
          min_dist[v] = d;
          parent[v] = u;
        }
      }
    }
  }
  return total;
}

double prim_mst_no_parent(const std::vector<Point> &points) {
  const int n = static_cast<int>(points.size());
  std::vector<bool> visited(n, false);
  std::vector<int> min_dist(n, INT_MAX);

  visited[0] = true;
  for (int i = 1; i < n; ++i)
    min_dist[i] = manhattan(points[0], points[i]);

  double total = 0.0;
  for (int k = 1; k < n; ++k) {
    int u = -1;
    int best = INT_MAX;
    for (int i = 0; i < n; ++i) {
      if (!visited[i] && min_dist[i] < best) {
        best = min_dist[i];
        u = i;
      }
    }
    if (u == -1)
      break;

    visited[u] = true;
    total += best;

    for (int v = 0; v < n; ++v) {
      if (!visited[v]) {
        int d = manhattan(points[u], points[v]);
        if (d < min_dist[v])
          min_dist[v] = d;
      }
    }
  }
  return total;
}

void get_hanan_candidates(const std::vector<Point> &points,
                          std::vector<int> &cand_x, std::vector<int> &cand_y) {
  std::vector<int> x_unique, y_unique;

  for (const auto &p : points) {
    if (std::find(x_unique.begin(), x_unique.end(), p.x) == x_unique.end())
      x_unique.push_back(p.x);
    if (std::find(y_unique.begin(), y_unique.end(), p.y) == y_unique.end())
      y_unique.push_back(p.y);
  }

  for (int x : x_unique) {
    for (int y : y_unique) {
      bool exists = false;
      for (const auto &p : points)
        if (p.x == x && p.y == y) {
          exists = true;
          break;
        }
      if (!exists) {
        cand_x.push_back(x);
        cand_y.push_back(y);
      }
    }
  }
}

void basic_algorithm(const std::vector<Point> &terminals,
                     std::vector<Point> &result_points,
                     std::vector<Edge> &result_edges) {
  result_points = terminals;
  int next_id = 0;
  for (const auto &p : result_points)
    if (p.id > next_id)
      next_id = p.id;
  ++next_id;

  while (true) {
    double cur_len = prim_mst_no_parent(result_points);

    std::vector<int> cand_x, cand_y;
    get_hanan_candidates(result_points, cand_x, cand_y);

    double best_gain = 0;
    int best_x = 0, best_y = 0;

    for (size_t i = 0; i < cand_x.size(); ++i) {
      std::vector<Point> temp_points = result_points;
      temp_points.push_back({0, cand_x[i], cand_y[i], 's'});

      double new_len = prim_mst_no_parent(temp_points);
      double gain = cur_len - new_len;
      if (gain > best_gain) {
        best_gain = gain;
        best_x = cand_x[i];
        best_y = cand_y[i];
      }
    }

    if (best_gain <= 0)
      break;

    result_points.push_back({next_id++, best_x, best_y, 's'});
  }

  std::vector<int> parent(result_points.size());
  prim_mst(result_points, parent);

  result_edges.clear();
  for (size_t i = 1; i < result_points.size(); ++i)
    result_edges.push_back({static_cast<int>(i), result_points[i].id,
                            result_points[parent[i]].id});
}

void write_output(const std::string &filename, const std::vector<Point> &points,
                  const std::vector<Edge> &edges) {
  json root;
  json node_arr = json::array();
  json edge_arr = json::array();

  for (const auto &p : points) {
    json node;
    node["id"] = p.id;
    node["x"] = p.x;
    node["y"] = p.y;
    node["type"] = (p.type == 't' ? "t" : "s");

    json edges_list = json::array();
    for (const auto &e : edges)
      if (e.u == p.id || e.v == p.id)
        edges_list.push_back(e.id);
    node["edges"] = edges_list;

    node_arr.push_back(node);
  }

  for (const auto &e : edges) {
    json edge;
    edge["id"] = e.id;
    edge["vertices"] = {e.u, e.v};
    edge_arr.push_back(edge);
  }

  root["node"] = node_arr;
  root["edge"] = edge_arr;

  std::ofstream f(filename);
  f << root.dump(4); // отступы для читаемости
}

std::vector<Point> read_input(const std::string &filename) {
  std::ifstream f(filename);
  if (!f.is_open()) {
    std::cerr << "Cannot open file " << filename << std::endl;
    return {};
  }

  json root;
  try {
    f >> root;
  } catch (const std::exception &e) {
    std::cerr << "JSON parse error: " << e.what() << std::endl;
    return {};
  }

  // Ищем массив "node"
  json node_arr;
  if (root.contains("node") && root["node"].is_array())
    node_arr = root["node"];
  else if (root.is_array())
    node_arr = root;
  else {
    std::cerr << "No 'node' array found" << std::endl;
    return {};
  }

  std::vector<Point> points;
  for (const auto &item : node_arr) {
    if (!item.contains("id") || !item.contains("x") || !item.contains("y")) {
      std::cerr << "Missing id/x/y in node" << std::endl;
      continue;
    }
    Point p;
    p.id = item["id"].get<int>();
    p.x = item["x"].get<int>();
    p.y = item["y"].get<int>();
    p.type = 't';
    points.push_back(p);
  }
  return points;
}

void parallel_algorithm(const std::vector<Point> &terminals,
                        std::vector<Point> &result_points,
                        std::vector<Edge> &result_edges) {
  omp_set_num_threads(NUM_THREADS);

  result_points = terminals;
  int next_id = 0;
  for (const auto &p : result_points)
    if (p.id > next_id)
      next_id = p.id;
  ++next_id;

  while (true) {
    double cur_len = prim_mst_no_parent(result_points);

    std::vector<int> cand_x, cand_y;
    get_hanan_candidates(result_points, cand_x, cand_y);

    double best_gain = 0;
    int best_x = 0, best_y = 0;

#pragma omp parallel
    {
      std::vector<Point> temp_points = result_points;
      temp_points.emplace_back(); // добавим пустую точку
      double local_best_gain = 0;
      int local_best_x = 0, local_best_y = 0;

#pragma omp for
      for (size_t i = 0; i < cand_x.size(); ++i) {
        temp_points.back() = {0, cand_x[i], cand_y[i], 's'};
        double new_len = prim_mst_no_parent(temp_points);
        double gain = cur_len - new_len;
        if (gain > local_best_gain) {
          local_best_gain = gain;
          local_best_x = cand_x[i];
          local_best_y = cand_y[i];
        }
      }

#pragma omp critical
      {
        if (local_best_gain > best_gain) {
          best_gain = local_best_gain;
          best_x = local_best_x;
          best_y = local_best_y;
        }
      }
    }

    if (best_gain <= 0)
      break;

    result_points.push_back({next_id++, best_x, best_y, 's'});
  }

  std::vector<int> parent(result_points.size());
  prim_mst(result_points, parent);

  result_edges.clear();
  for (size_t i = 1; i < result_points.size(); ++i)
    result_edges.push_back({static_cast<int>(i), result_points[i].id,
                            result_points[parent[i]].id});
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " [-m] <test.json>\n";
    return 1;
  }

  bool modified = false;
  std::string filename;

  if (std::strcmp(argv[1], "-m") == 0) {
    if (argc < 3) {
      std::cerr << "Missing filename after -m\n";
      return 1;
    }
    modified = true;
    filename = argv[2];
  } else {
    filename = argv[1];
  }

  std::vector<Point> terminals = read_input(filename);
  if (terminals.empty())
    return 1;

  std::vector<Point> result_points;
  std::vector<Edge> result_edges;

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  if (modified)
    parallel_algorithm(terminals, result_points, result_edges);
  else
    basic_algorithm(terminals, result_points, result_edges);

  clock_gettime(CLOCK_MONOTONIC, &end);
  double elapsed =
      (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

  // Формируем имя выходного файла
  std::string outfile = filename;
  size_t dot_pos = outfile.rfind(".json");
  if (dot_pos != std::string::npos)
    outfile = outfile.substr(0, dot_pos) + "_out.json";
  else
    outfile += "_out.json";

  write_output(outfile, result_points, result_edges);

  int total_len = 0;
  for (const auto &e : result_edges) {
    int ui = find_point_by_id(result_points, e.u);
    int vi = find_point_by_id(result_points, e.v);
    if (ui != -1 && vi != -1)
      total_len += manhattan(result_points[ui], result_points[vi]);
  }

  std::cout << "time [" << elapsed * 1000 << " ms] length [" << total_len
            << "]\n";

  return 0;
}
