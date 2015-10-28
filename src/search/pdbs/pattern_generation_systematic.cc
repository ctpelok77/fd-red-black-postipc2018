#include "pattern_generation_systematic.h"

#include "canonical_pdbs_heuristic.h"

#include "../causal_graph.h"
#include "../globals.h"
#include "../plugin.h"
#include "../task_tools.h"

#include <algorithm>
#include <iostream>

using namespace std;


static bool patterns_are_disjoint(
    const Pattern &pattern1, const Pattern &pattern2) {
    size_t i = 0;
    size_t j = 0;
    for (;;) {
        if (i == pattern1.size() || j == pattern2.size())
            return true;
        int val1 = pattern1[i];
        int val2 = pattern2[j];
        if (val1 == val2)
            return false;
        else if (val1 < val2)
            ++i;
        else
            ++j;
    }
}

static void compute_union_pattern(
    const Pattern &pattern1, const Pattern &pattern2, Pattern &result) {
    result.clear();
    result.reserve(pattern1.size() + pattern2.size());
    set_union(pattern1.begin(), pattern1.end(),
              pattern2.begin(), pattern2.end(),
              back_inserter(result));
}


PatternGenerationSystematicNaive::PatternGenerationSystematicNaive(
        const Options &opts)
        : task(get_task_from_options(opts)),
          task_proxy(*task) {
    int pattern_max_size = opts.get<int>("pattern_max_size");
    int num_variables = task_proxy.get_variables().size();
    vector<vector<int> > current_patterns(1);
    vector<vector<int> > next_patterns;
    for (int i = 0; i < pattern_max_size; ++i) {
        for (const vector<int> &current_pattern : current_patterns) {
            int max_var = -1;
            if (i > 0)
                max_var = current_pattern.back();
            for (int var = max_var + 1; var < num_variables; ++var) {
                vector<int> pattern = current_pattern;
                pattern.push_back(var);
                next_patterns.push_back(pattern);
                patterns.push_back(pattern);
            }
        }
        next_patterns.swap(current_patterns);
        next_patterns.clear();
    }

    cout << "Found " << patterns.size() << " patterns." << endl;
}

PatternGenerationSystematicNaive::~PatternGenerationSystematicNaive() {
}

CanonicalPDBsHeuristic *PatternGenerationSystematicNaive::get_pattern_collection_heuristic(const Options &opts) const {
    Options canonical_opts;
    canonical_opts.set<int>("cost_type", OperatorCost(opts.get<int>("cost_type")));
    canonical_opts.set("patterns", patterns);
    CanonicalPDBsHeuristic *h = new CanonicalPDBsHeuristic(canonical_opts);
    if (opts.get<bool>("dominance_pruning")) {
        h->dominance_pruning();
    }
    return h;
}


PatternGenerationSystematic::PatternGenerationSystematic(
    const Options &opts)
    : task(get_task_from_options(opts)),
      task_proxy(*task),
      max_pattern_size(opts.get<int>("pattern_max_size")) {
    build_patterns();
}

PatternGenerationSystematic::~PatternGenerationSystematic() {
}

void PatternGenerationSystematic::compute_eff_pre_neighbors(
    const CausalGraph &cg, const Pattern &pattern, vector<int> &result) const {
    /*
      Compute all variables that are reachable from pattern by an
      (eff, pre) arc and are not already contained in the pattern.
    */
    unordered_set<int> candidates;

    // Compute neighbors.
    for (int var : pattern) {
        const vector<int> &neighbors = cg.get_eff_to_pre(var);
        candidates.insert(neighbors.begin(), neighbors.end());
    }

    // Remove elements of pattern.
    for (int var : pattern) {
        candidates.erase(var);
    }

    result.assign(candidates.begin(), candidates.end());
}

void PatternGenerationSystematic::compute_connection_points(
    const CausalGraph &cg, const Pattern &pattern, vector<int> &result) const {
    /*
      The "connection points" of a pattern are those variables of which
      one must be contained in an SGA pattern that can be attached to this
      pattern to form a larger interesting pattern. (Interesting patterns
      are disjoint unions of SGA patterns.)

      A variable is a connection point if it satisfies the following criteria:
      1. We can get from the pattern to the connection point via
         an (eff, pre) or (eff, eff) arc in the causal graph.
      2. It is not part of pattern.
      3. We *cannot* get from the pattern to the connection point
         via an (eff, pre) arc.

      Condition 1. is the important one. The other conditions are
      optimizations that help reduce the number of candidates to
      consider.
    */
    unordered_set<int> candidates;

    // Handle rule 1.
    for (int var : pattern) {
        const vector<int> &pred = cg.get_predecessors(var);
        candidates.insert(pred.begin(), pred.end());
    }

    // Handle rules 2 and 3.
    for (int var : pattern) {
        // Rule 2:
        candidates.erase(var);
        // Rule 3:
        const vector<int> &eff_pre = cg.get_eff_to_pre(var);
        for (int pre_var : eff_pre)
            candidates.erase(pre_var);
    }

    result.assign(candidates.begin(), candidates.end());
}

void PatternGenerationSystematic::enqueue_pattern_if_new(const Pattern &pattern) {
    if (pattern_set.insert(pattern).second)
        patterns.push_back(pattern);
}

void PatternGenerationSystematic::build_sga_patterns(const CausalGraph &cg) {
    assert(max_pattern_size >= 1);
    assert(pattern_set.empty());
    assert(patterns.empty());

    /*
      SGA patterns are "single-goal ancestor" patterns, i.e., those
      patterns which can be generated by following eff/pre arcs from a
      single goal variable.

      This method must generate all SGA patterns up to size
      "max_pattern_size". They must be generated in order of
      increasing size, and they must be placed in "patterns".

      The overall structure of this is a similar processing queue as
      in the main pattern generation method below, and we reuse
      "patterns" and "pattern_set" between the two methods.
    */

    // Build goal patterns.
    for (FactProxy goal : task_proxy.get_goals()) {
        int var_id = goal.get_variable().get_id();
        Pattern goal_pattern;
        goal_pattern.push_back(var_id);
        enqueue_pattern_if_new(goal_pattern);
    }

    /*
      Grow SGA patterns untill all patterns are processed. Note that
      the patterns vectors grows during the computation.
    */
    for (size_t pattern_no = 0; pattern_no < patterns.size(); ++pattern_no) {
        // We must copy the pattern because references to patterns can be invalidated.
        Pattern pattern = patterns[pattern_no];
        if (pattern.size() == max_pattern_size)
            break;

        vector<int> neighbors;
        compute_eff_pre_neighbors(cg, pattern, neighbors);

        for (int neighbor_var_id : neighbors) {
            Pattern new_pattern(pattern);
            new_pattern.push_back(neighbor_var_id);
            sort(new_pattern.begin(), new_pattern.end());

            enqueue_pattern_if_new(new_pattern);
        }
    }

    pattern_set.clear();
}

void PatternGenerationSystematic::build_patterns() {
    int num_variables = task_proxy.get_variables().size();
    const CausalGraph &cg = task_proxy.get_causal_graph();

    // Generate SGA (single-goal-ancestor) patterns.
    // They are generated into the patterns variable,
    // so we swap them from there.
    build_sga_patterns(cg);
    vector<Pattern> sga_patterns;
    patterns.swap(sga_patterns);

    /* Index the SGA patterns by variable.

       Important: sga_patterns_by_var[var] must be sorted by size.
       This is guaranteed because build_sga_patterns generates
       patterns ordered by size.
    */
    vector<vector<const Pattern *> > sga_patterns_by_var(num_variables);
    for (const Pattern &pattern : sga_patterns) {
        for (int var : pattern) {
            sga_patterns_by_var[var].push_back(&pattern);
        }
    }

    // Enqueue the SGA patterns.
    for (const Pattern &pattern : sga_patterns)
        enqueue_pattern_if_new(pattern);


    cout << "Found " << sga_patterns.size() << " SGA patterns." << endl;

    /*
      Combine patterns in the queue with SGA patterns until all
      patterns are processed. Note that the patterns vectors grows
      during the computation.
    */
    for (size_t pattern_no = 0; pattern_no < patterns.size(); ++pattern_no) {
        // We must copy the pattern because references to patterns can be invalidated.
        Pattern pattern1 = patterns[pattern_no];

        vector<int> neighbors;
        compute_connection_points(cg, pattern1, neighbors);

        for (int neighbor_var : neighbors) {
            const auto &candidates = sga_patterns_by_var[neighbor_var];
            for (const Pattern *p_pattern2 : candidates) {
                const Pattern &pattern2 = *p_pattern2;
                if (pattern1.size() + pattern2.size() > max_pattern_size)
                    break;  // All remaining candidates are too large.
                if (patterns_are_disjoint(pattern1, pattern2)) {
                    Pattern new_pattern;
                    compute_union_pattern(pattern1, pattern2, new_pattern);
                    enqueue_pattern_if_new(new_pattern);
                }
            }
        }
    }

    pattern_set.clear();
    cout << "Found " << patterns.size() << " interesting patterns." << endl;

    /*
    cout << "list of patterns:" << endl;
    for (size_t i = 0; i < patterns.size(); ++i)
        cout << patterns[i] << endl;
    */
}

const vector<Pattern> &PatternGenerationSystematic::get_patterns() const {
    return patterns;
}

CanonicalPDBsHeuristic *PatternGenerationSystematic::get_pattern_collection_heuristic(const Options &opts) const {
    Options canonical_opts;
    canonical_opts.set<shared_ptr<AbstractTask>>(
        "transform", opts.get<shared_ptr<AbstractTask>>("transform"));
    canonical_opts.set("patterns", patterns);
    CanonicalPDBsHeuristic *h = new CanonicalPDBsHeuristic(canonical_opts);
    if (opts.get<bool>("dominance_pruning")) {
        h->dominance_pruning();
    }
    return h;
}

void PatternGenerationSystematic::add_systematic_pattern_options(
    OptionParser &parser) {
    parser.add_option<int>(
        "pattern_max_size",
        "max number of variables per pattern",
        "1");
    parser.add_option<bool>(
        "only_interesting_patterns",
        "Only consider the union of two disjoint patterns if the union has "
        "more information than the individual patterns.",
        "true");
}

void PatternGenerationSystematic::check_systematic_pattern_options(
    OptionParser &parser, const Options &opts) {
    if (opts.get<int>("pattern_max_size") < 1)
        parser.error("number of variables per pattern must be at least 1");
}


static ScalarEvaluator *_parse(OptionParser &parser) {
    PatternGenerationSystematic::add_systematic_pattern_options(parser);
    parser.add_option<bool>(
        "dominance_pruning",
        "Use dominance pruning to reduce number of patterns.",
        "true");
    Heuristic::add_options_to_parser(parser);

    Options opts = parser.parse();

    PatternGenerationSystematic::check_systematic_pattern_options(parser, opts);
    if (parser.dry_run())
        return 0;

    if (opts.get<bool>("only_interesting_patterns")) {
        PatternGenerationSystematic pattern_generator(opts);
        return pattern_generator.get_pattern_collection_heuristic(opts);
    } else {
        PatternGenerationSystematicNaive pattern_generator(opts);
        return pattern_generator.get_pattern_collection_heuristic(opts);
    }
}

static Plugin<ScalarEvaluator> _plugin("cpdbs_systematic", _parse);
