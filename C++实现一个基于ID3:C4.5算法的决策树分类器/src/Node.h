#ifndef NODE_H
#define NODE_H

#include <vector>
#include <string>
#include <memory>

class Node {
public:
    bool isLeaf;                     // 是否为叶子节点
    int attributeIndex;              // 分裂属性的索引
    double splitValue;               // 分裂值
    std::string className;           // 类别名称（叶子节点）
    std::vector<std::shared_ptr<Node>> children;  // 子节点

    Node(bool leaf = false) : 
        isLeaf(leaf), 
        attributeIndex(-1), 
        splitValue(0.0) {}
};

#endif // NODE_H 