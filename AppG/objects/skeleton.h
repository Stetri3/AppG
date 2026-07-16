#pragma once
#include "../math/linalg/vectors.h"
#include <vector>
#include <cstdint>
#include <span>

namespace sk {

	struct BodyAttach {
		Vec3f pos;
		uint32_t index; // Index of the base node in the 'nodes' array
	};

	struct Node {
		// --- Relative (Local) Transforms ---
		Mat3f local_ori; // Orientation relative to parent
		Vec3f local_pos; // Translation relative to parent

		// --- Absolute (Global) Transforms (Cached) ---
		Mat3f abs_ori;   // World space orientation (Sent to GPU)
		Vec3f abs_pos;   // World space position (Sent to GPU)

		uint8_t branch;  // 1 -> stay on same level, 0 -> go down, 2 -> up a level
		// (1 << 7) & branch -> base node (attached to body)
		uint32_t subtree_size;
	};

	class Skeleton {
		std::vector<Node> nodes;          // Ordered top-down (DFS)
		std::vector<BodyAttach> attached; // Base node attachments
		uint64_t id;

	public:
		virtual ~Skeleton() = default;

		// Recomputes all absolute transforms in a single flat O(N) pass
		void update_absolute_transforms();

		std::span<Node> get_subTree(uint32_t index);
		bool move_manual(std::span<Vec3f> local_pos, std::span<Mat3f> local_ori, uint32_t index = 0);

		// These now run in O(1) time!
		void rotate_subTree(uint32_t root_index, const Mat3f& rotation_matrix);
		void transform_subTree(uint32_t root_index, const Mat4f& local_transform);
	};
}