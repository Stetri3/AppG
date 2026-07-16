#include "skeleton.h"
#include <algorithm>

namespace sk {

	// Struct to keep track of the parent state during our flat DFS traversal
	struct ParentState {
		Mat3f ori;
		Vec3f pos;
	};
	// Recalculates world-space matrices in a single O(N) pass
	void Skeleton::update_absolute_transforms() {
		if (nodes.empty()) return;

		// Flat array acting as a stack to track parent transforms down the DFS depth.
		// A size of 64 easily covers any realistic skeletal hierarchy depth.
		ParentState parent_stack[64];
		int stack_ptr = 0;

		// Seed a default global root state
		parent_stack[0] = { Mat3f::One(), Vec3f{0.0f, 0.0f, 0.0f} };

		for (size_t i = 0; i < nodes.size(); ++i) {
			Node& node = nodes[i];
			ParentState current_parent = parent_stack[stack_ptr];

			// 1. Check if this node is attached to the body (base node)
			if ((node.branch & (1 << 7)) != 0) {
				// Find matching body attachment
				for (const auto& att : attached) {
					if (att.index == i) {
						current_parent.pos = att.pos;
						current_parent.ori = Mat3f::One(); // Base nodes align with body
						break;
					}
				}
			}

			// 2. Compute absolute orientation: ParentOri * LocalOri
			node.abs_ori = current_parent.ori * node.local_ori;

			// 3. Compute absolute position: ParentPos + ParentOri * LocalPos
			node.abs_pos = current_parent.pos + (current_parent.ori * node.local_pos);

			// 4. Update DFS stack based on branch instruction
			uint8_t branch_type = node.branch & 0x7F; // Strip base node bit

			if (branch_type == 0) {
				// Go down a level: This node becomes the parent for the next nodes
				if (stack_ptr < 63) {
					stack_ptr++;
					parent_stack[stack_ptr] = { node.abs_ori, node.abs_pos };
				}
			}
			else if (branch_type == 2) {
				// Go up a level: Pop current parent
				if (stack_ptr > 0) {
					stack_ptr--;
				}
			}
			// branch_type == 1 (stay on same level) leaves the stack pointer untouched
		}
	}

	std::span<Node> Skeleton::get_subTree(uint32_t index) {
		if (index >= nodes.size()) return {};
		uint32_t size = nodes[index].subtree_size;
		if (index + size > nodes.size()) {
			size = static_cast<uint32_t>(nodes.size() - index);
		}
		return std::span<Node>(nodes.data() + index, size);
	}

	// Writes directly to local coordinate registers
	bool Skeleton::move_manual(std::span<Vec3f> local_pos, std::span<Mat3f> local_ori, uint32_t index) {
		auto sub_tree = get_subTree(index);
		if (sub_tree.empty()) return false;

		size_t update_size = std::min({ sub_tree.size(), local_pos.size(), local_ori.size() });

		for (size_t i = 0; i < update_size; ++i) {
			sub_tree[i].local_pos = local_pos[i];
			sub_tree[i].local_ori = local_ori[i];
		}

		// Flag/Trigger update pass after manual edits
		update_absolute_transforms();
		return true;
	}

	// O(1) Rotation: Only the root of the subtree needs its local frame rotated!
	void Skeleton::rotate_subTree(uint32_t root_index, const Mat3f& rotation_matrix) {
		if (root_index >= nodes.size()) return;

		// Transform the local coordinate space of the root node
		nodes[root_index].local_ori = rotation_matrix * nodes[root_index].local_ori;

		// Defer absolute position updates to the O(N) linear sweep
		update_absolute_transforms();
	}

	// O(1) Affine Transform: Mutates the local relative channel of the root
	void Skeleton::transform_subTree(uint32_t root_index, const Mat4f& local_transform) {
		if (root_index >= nodes.size()) return;

		Mat3f R;
		for (int r = 0; r < 3; ++r) {
			for (int c = 0; c < 3; ++c) {
				R(r, c) = local_transform(r, c);
			}
		}
		Vec3f T{ local_transform(0, 3), local_transform(1, 3), local_transform(2, 3) };

		// Rotate the local orientation frame
		nodes[root_index].local_ori = R * nodes[root_index].local_ori;

		// Offset local translation relative to parent frame
		nodes[root_index].local_pos = (R * nodes[root_index].local_pos) + T;

		// Propagate relative changes down the hierarchy
		update_absolute_transforms();
	}
}