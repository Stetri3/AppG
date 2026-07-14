#include "skeleton.h"
namespace sk {

	// Ottiene lo span del sottoalbero in O(1) sfruttando subtree_size
	std::span<Node> Skeleton::get_subTree(uint32_t index) {
		if (index >= nodes.size()) {
			return {};
		}

		uint32_t size = nodes[index].subtree_size;
		// Sicurezza contro out-of-bounds se subtree_size non è coerente
		if (index + size > nodes.size()) {
			size = static_cast<uint32_t>(nodes.size() - index);
		}

		return std::span<Node>(nodes.data() + index, size);
	}

	// Sovrascrive posizioni e orientamenti locali dei nodi nel sottoalbero
	bool Skeleton::move_manual(std::span<Vec3f> pos, std::span<Mat3f> ori, uint32_t index) {
		auto sub_tree = get_subTree(index);
		if (sub_tree.empty()) {
			return false;
		}

		// Possiamo aggiornare solo fino al limite degli span passati in input
		size_t update_size = std::min({ sub_tree.size(), pos.size(), ori.size() });

		for (size_t i = 0; i < update_size; ++i) {
			sub_tree[i].pos = pos[i];
			sub_tree[i].ori = ori[i];
		}

		return true;
	}

	// Ruota l'intero sottoalbero rispetto alla posizione del suo nodo radice (root_index)
	void Skeleton::rotate_subTree(uint32_t root_index, const Mat3f& rotation_matrix) {
		auto sub_tree = get_subTree(root_index);
		if (sub_tree.empty()) {
			return;
		}

		// La rotazione del sottoalbero avviene attorno alla posizione della radice locale
		Vec3f pivot = sub_tree[0].pos;

		for (Node& node : sub_tree) {
			// 1. Ruota l'orientamento del nodo: R * Ori
			node.ori = rotation_matrix * node.ori;

			// 2. Ruota la posizione del nodo rispetto al pivot
			// pos_nuova = pivot + R * (pos_attuale - pivot)
			Vec3f local_pos = node.pos - pivot;
			node.pos = pivot + (rotation_matrix * local_pos);
		}
	}

	// Applica una trasformazione affine 4x4 (rotazione + traslazione) al sottoalbero
	void Skeleton::transform_subTree(uint32_t root_index, const Mat4f& local_transform) {
		auto sub_tree = get_subTree(root_index);
		if (sub_tree.empty()) {
			return;
		}

		// Estraiamo la parte di rotazione 3x3 e traslazione 3x1 dalla matrice 4x4
		// Nota: Assumiamo che la tua classe Mat4f esponga l'accesso agli elementi o costruttori di conversione.
		// Se la libreria non lo supporta direttamente, adatta l'estrazione degli indici.
		Mat3f R;
		for (int r = 0; r < 3; ++r) {
			for (int c = 0; c < 3; ++c) {
				R(r, c) = local_transform(r, c);
			}
		}

		Vec3f T{ local_transform(0, 3), local_transform(1, 3), local_transform(2, 3) };
		Vec3f pivot = sub_tree[0].pos;

		for (Node& node : sub_tree) {
			// 1. Applica la rotazione all'orientamento
			node.ori = R * node.ori;

			// 2. Trasforma la posizione relativa al pivot:
			// pos_nuova = pivot + R * (pos_attuale - pivot) + T
			Vec3f local_pos = node.pos - pivot;
			node.pos = pivot + (R * local_pos) + T;
		}
	}
}