#include <iostream>
#include <vector>
#include <Eigen/Dense>
#include <cmath>
#include <random>

using namespace std;
using namespace Eigen;

// 图结构的定义
class Graph {
private:
    int num_nodes;
    vector<vector<int> > adjacency_list;
    MatrixXd node_features;
    
public:
    Graph(int n) : num_nodes(n) {
        adjacency_list.resize(n);
        node_features = MatrixXd::Zero(n, 0);
    }
    
    void add_edge(int from, int to) {
        if (from >= 0 && from < num_nodes && to >= 0 && to < num_nodes) {
            adjacency_list[from].push_back(to);
            adjacency_list[to].push_back(from); // 无向图
        }
    }
    
    void set_node_features(const MatrixXd& features) {
        node_features = features;
    }
    
    MatrixXd get_node_features() const {
        return node_features;
    }
    
    vector<vector<int> > get_adjacency_list() const {
        return adjacency_list;
    }
    
    int get_num_nodes() const {
        return num_nodes;
    }
};

// GNN层的定义
class GNNLayer {
private:
    MatrixXd weight_matrix;
    int input_dim;
    int output_dim;
    
public:
    GNNLayer(int in_dim, int out_dim) : input_dim(in_dim), output_dim(out_dim) {
        // 随机初始化权重
        random_device rd;
        mt19937 gen(rd());
        normal_distribution<double> distribution(0.0, 0.1);
        
        weight_matrix = MatrixXd::Zero(input_dim, output_dim);
        for(int i = 0; i < input_dim; i++) {
            for(int j = 0; j < output_dim; j++) {
                weight_matrix(i, j) = distribution(gen);
            }
        }
    }
    
    // ReLU激活函数
    static double relu(double x) {
        return max(0.0, x);
    }
    
    MatrixXd forward(const Graph& graph) {
        MatrixXd features = graph.get_node_features();
        vector<vector<int> > adj_list = graph.get_adjacency_list();
        int num_nodes = graph.get_num_nodes();
        
        // 对每个节点，聚合邻居的特征
        MatrixXd aggregated_features = MatrixXd::Zero(num_nodes, input_dim);
        for(int i = 0; i < num_nodes; i++) {
            // 包含自身特征
            aggregated_features.row(i) = features.row(i);
            
            // 聚合邻居特征
            for(size_t j = 0; j < adj_list[i].size(); j++) {
                int neighbor = adj_list[i][j];
                aggregated_features.row(i) += features.row(neighbor);
            }
            
            // 归一化
            aggregated_features.row(i) /= (adj_list[i].size() + 1.0);
        }
        
        // 线性变换
        MatrixXd transformed = aggregated_features * weight_matrix;
        
        // 应用ReLU激活函数
        for(int i = 0; i < transformed.rows(); i++) {
            for(int j = 0; j < transformed.cols(); j++) {
                transformed(i, j) = relu(transformed(i, j));
            }
        }
        
        return transformed;
    }
};

int main() {
    // 创建示例图
    Graph graph(4); // 4个节点的图
    
    // 添加边
    graph.add_edge(0, 1);
    graph.add_edge(1, 2);
    graph.add_edge(2, 3);
    graph.add_edge(3, 0);
    
    // 设置初始节点特征（每个节点3维特征）
    MatrixXd initial_features(4, 3);
    initial_features << 1.0, 0.0, 0.0,
                       0.0, 1.0, 0.0,
                       0.0, 0.0, 1.0,
                       1.0, 1.0, 1.0;
    graph.set_node_features(initial_features);
    
    // 创建GNN层（输入维度3，输出维度2）
    GNNLayer gnn_layer(3, 2);
    
    // 执行前向传播
    cout << "初始节点特征：" << endl;
    cout << graph.get_node_features() << endl << endl;
    
    MatrixXd output = gnn_layer.forward(graph);
    
    cout << "GNN层输出特征：" << endl;
    cout << output << endl;
    
    return 0;
} 