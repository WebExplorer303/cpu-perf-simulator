#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

static uint64_t parse_address(const std::string& s) {
    return std::stoull(s, nullptr, 0);
}

struct CacheLine { uint64_t tag; };
struct CacheStats { uint64_t accesses=0, hits=0, misses=0; };

static int run_cache(const std::string& path, uint64_t capacity, uint64_t ways, uint64_t block) {
    if (!capacity || !ways || !block || capacity % (ways * block) != 0) {
        std::cerr << "Invalid cache geometry: capacity must be divisible by ways * block size.\n";
        return 2;
    }
    const uint64_t sets_count = capacity / (ways * block);
    std::vector<std::list<CacheLine>> sets(sets_count); // front = most recently used
    CacheStats stats;
    std::ifstream in(path);
    if (!in) { std::cerr << "Cannot open trace: " << path << "\n"; return 2; }

    char op;
    std::string address_text;
    while (in >> op >> address_text) {
        if (op != 'R' && op != 'W') {
            std::cerr << "Skipping invalid operation: " << op << "\n";
            continue;
        }
        uint64_t address;
        try { address = parse_address(address_text); }
        catch (...) { std::cerr << "Skipping invalid address: " << address_text << "\n"; continue; }

        const uint64_t block_num = address / block;
        const uint64_t set_index = block_num % sets_count;
        const uint64_t tag = block_num / sets_count;
        auto& set = sets[set_index];
        auto it = std::find_if(set.begin(), set.end(), [tag](const CacheLine& l){ return l.tag == tag; });
        ++stats.accesses;
        if (it != set.end()) {
            ++stats.hits;
            set.splice(set.begin(), set, it);
        } else {
            ++stats.misses;
            if (set.size() >= ways) set.pop_back();
            set.push_front(CacheLine{tag});
        }
    }
    const double hit_rate = stats.accesses ? 100.0 * stats.hits / stats.accesses : 0.0;
    const double miss_rate = stats.accesses ? 100.0 * stats.misses / stats.accesses : 0.0;
    std::cout << "Cache summary\nAccesses: " << stats.accesses
              << "\nHits: " << stats.hits << "\nMisses: " << stats.misses
              << "\nHit rate: " << std::fixed << std::setprecision(2) << hit_rate << "%"
              << "\nMiss rate: " << miss_rate << "%\n";
    return 0;
}

static int run_predictor(const std::string& path, const std::string& mode, size_t entries) {
    if ((mode != "bimodal" && mode != "gshare") || entries < 2 ||
        (entries & (entries - 1)) != 0) {
        std::cerr << "Predictor mode must be bimodal/gshare and table size a power of two >= 2.\n";
        return 2;
    }
    std::ifstream in(path);
    if (!in) { std::cerr << "Cannot open trace: " << path << "\n"; return 2; }
    std::vector<uint8_t> counters(entries, 1); // weakly not-taken, 2-bit saturating
    const unsigned bits = [&] { unsigned b=0; for (size_t n=entries; n>1; n>>=1) ++b; return b; }();
    uint32_t global_history = 0;
    uint64_t total = 0, correct = 0;
    std::string outcome;
    while (in >> outcome) {
        if (outcome != "T" && outcome != "N" && outcome != "t" && outcome != "n") continue;
        const bool taken = (outcome == "T" || outcome == "t");
        const size_t index = mode == "bimodal"
            ? (static_cast<size_t>(total) & (entries - 1))
            : ((static_cast<size_t>(total) ^ global_history) & (entries - 1));
        const bool prediction = counters[index] >= 2;
        ++total;
        if (prediction == taken) ++correct;
        if (taken) { if (counters[index] < 3) ++counters[index]; }
        else { if (counters[index] > 0) --counters[index]; }
        global_history = ((global_history << 1) | (taken ? 1u : 0u)) & ((1u << bits) - 1u);
    }
    const double accuracy = total ? 100.0 * correct / total : 0.0;
    const double mpki = total ? 1000.0 * (total - correct) / total : 0.0;
    std::cout << mode << " predictor summary\nBranches: " << total
              << "\nCorrect: " << correct << "\nMispredictions: " << (total-correct)
              << "\nAccuracy: " << std::fixed << std::setprecision(2) << accuracy << "%"
              << "\nMispredictions per 1K branches: " << mpki << "\n";
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage:\n  cpu-sim cache <trace> <capacity_bytes> <ways> <block_bytes>\n"
                  << "  cpu-sim predict <trace> <bimodal|gshare> <table_entries>\n";
        return 2;
    }
    const std::string command = argv[1];
    try {
        if (command == "cache" && argc == 6)
            return run_cache(argv[2], std::stoull(argv[3]), std::stoull(argv[4]), std::stoull(argv[5]));
        if (command == "predict" && argc == 5)
            return run_predictor(argv[2], argv[3], std::stoull(argv[4]));
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
    std::cerr << "Invalid arguments. See README for usage.\n";
    return 2;
}
