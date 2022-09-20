#ifndef __PANEL_NODE_EDITOR_H__
#define __PANEL_NODE_EDITOR_H__

#include "Panel.h"
#include <vector>
class M_Editor;

#include "Node.h"

struct Link
{
	int id;
	int start_attr, end_attr;
	int start_node, end_node;
};


class PanelNodeEditor : public Panel {
public:
	PanelNodeEditor(M_Editor* editor);
	~PanelNodeEditor();

	bool Awake();
	bool Update();

private:
	//Node editor functions
	int CreateNode(NodeType type);
	void DrawNodes(Node* node);
	void RightClickListener();
	void NodeHoveringAndSelectingListener();
	void SaveNodeEditor();
	void LoadNodeEditor(const char* path);
	Node* FindNode(int id) {
		for (auto& node : nodes) {
			if (node->id == id) {
				return node;
			}
		}
		return nullptr;
	}

	Link* FindLink(int id) {
		for (auto& link : links) {
			if (link.id == id) {
				return &link;
			}
		}
		return nullptr;
	}
	Pin* FindPin(int id) {
		if (!id) {
			return nullptr;
		}
		for (auto& node : nodes) {
			for (auto& pin : node->inputPins) {
				if (pin.id == id) {
					return &pin;
				}
			}
			for (auto& pin : node->outputPins) {
				if (pin.id == id) {
					return &pin;
				}
			}
		}
		return nullptr;
	}
private:
	M_Editor* editor;
	std::vector<Node*> nodes;
	std::vector<Link> links;
	std::vector<int> selectedNodes;

	int currentId;
	int hoveredId = 0;

};

#endif // !__PANEL_NODE_EDITOR_H__