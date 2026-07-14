#pragma once
#include "../math/vectors.h"
#include <vector>
#include <cstdint>
#include <span>
namespace sk {


	struct BodyAttach {
		Vec3f pos;
		uint32_t index;
	};
	struct Node {
		Mat3f ori; //Orientamento
		Vec3f pos;
		uint8_t branch; //1 -> stay on the same node level, 0-> go down a level, 2-> up a level
		//1<<7 & branch -> base node (attached to body)
		uint32_t subtree_size;
	};
	class Skeleton {
		//Ottimizzata per lettura, scrittura trascurabile
		std::vector<Node> nodes; //Ordered top down ALWAYS (add/remove time negligible)
		std::vector<BodyAttach> attached; //position of base nodes
		uint64_t id;
	public:
		virtual ~Skeleton() = default;
		std::span<Node> get_subTree(uint32_t index);
		bool move_manual(std::span<Vec3f> pos, std::span<Mat3f> ori, uint32_t index = 0);
		void rotate_subTree(uint32_t root_index, const Mat3f& rotation_matrix);
		void transform_subTree(uint32_t root_index, const Mat4f& local_transform);
	};
}