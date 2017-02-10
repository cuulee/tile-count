#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include <map>
#include <vector>

// Karnin-Lang-Liberty streaming quantile estimation
// https://arxiv.org/abs/1603.05346

template <typename T>
class kll {
	std::vector<std::vector<T>> compactors;
	size_t k = 512;
	double c = 2.0 / 3.0;
	size_t H = 0;
	size_t size = 0;
	size_t maxSize = 0;
	size_t zeroes = 0;

       public:
	kll() {
		grow();
	}

	void grow() {
		compactors.push_back(std::vector<T>());
		H = compactors.size();

		maxSize = 0;
		for (size_t h = 0; h < H; h++) {
			maxSize += capacity(h);
		}
	}

	size_t capacity(size_t h) {
		return ceil(k * exp(log(c) * (H - h - 1))) + 1;
	}

	void update(T value) {
		if (value == 0) {
			zeroes++;
		} else {
			compactors[0].push_back(value);
			size++;
			while (size >= maxSize) {
				compact();
			}
		}
	}

	void compact() {
		size_t s = compactors.size();
		for (size_t h = 0; h < s; h++) {
			if (compactors[h].size() >= capacity(h)) {
				if (h + 1 >= H) {
					grow();
				}

				std::vector<T> compacted = compact(compactors[h]);
				compactors[h + 1].insert(compactors[h + 1].end(), compacted.begin(), compacted.end());

				size = 0;
				for (size_t i = 0; i < compactors.size(); i++) {
					size = compactors[i].size();
				}

				break;
			}
		}
	}

	void merge(kll<T> &t) {
		zeroes += t.zeroes;

		while (H < t.H) {
			grow();
		}

		for (size_t h = 0; h < t.compactors.size(); h++) {
			compactors[h].insert(compactors[h].end(), t.compactors[h].begin(), t.compactors[h].end());
		}

		size = 0;
		for (size_t i = 0; i < compactors.size(); i++) {
			size += compactors[i].size();
		}

		while (size >= maxSize) {
			compact();
		}
	}

	std::vector<T> compact(std::vector<T> &v) {
		std::sort(v.begin(), v.end());
		bool odd = rand() % 1;

		std::vector<T> ret;

		while (v.size() >= 2) {
			if (odd) {
				ret.push_back(v[v.size() - 1]);
				v.pop_back();
				v.pop_back();
			} else {
				ret.push_back(v[v.size() - 2]);
				v.pop_back();
				v.pop_back();
			}
		}

		return ret;
	}

	std::vector<std::pair<double, T>> cdf() {
		std::multimap<T, double> iw;

		double total_weight = 0;

		iw.insert(std::pair<T, double>(0, zeroes));
		total_weight += zeroes;

		for (size_t i = 0; i < compactors.size(); i++) {
			for (size_t j = 0; j < compactors[i].size(); j++) {
				double weight = exp(log(2) * i);
				iw.insert(std::pair<T, double>(compactors[i][j], weight));
				total_weight += weight;
			}
		}

		std::vector<std::pair<double, T>> ret;
		double cumulative = 0;

		for (auto a = iw.begin(); a != iw.end(); a++) {
			cumulative += a->second;
			ret.push_back(std::pair<double, T>(cumulative / total_weight, a->first));
		}

		return ret;
	}
};