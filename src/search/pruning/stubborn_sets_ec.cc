#include "stubborn_sets_ec.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_tools.h"

#include "../utils/markup.h"
#include "../utils/collections.h"

#include <cassert>
#include <unordered_map>

using namespace std;

namespace stubborn_sets_ec {
// DTGs are stored as one adjacency list per value.
using StubbornDTG = vector<vector<int>>;

static inline bool is_v_applicable(int var,
                                   int op_no,
                                   const State &state,
                                   vector<vector<int>> &preconditions) {
    int precondition_on_var = preconditions[op_no][var];
    return precondition_on_var == -1 || precondition_on_var == state[var].get_value();
}

vector<StubbornDTG> build_dtgs(TaskProxy task_proxy) {
    /*
      NOTE: Code lifted and adapted from M&S atomic abstraction code.
      We need a more general mechanism for creating data structures of
      this kind.
     */

    /*
      NOTE: for stubborn sets ec, the DTG for v *does* include
      self-loops from d to d if there is an operator that sets the
      value of v to d and has no precondition on v. This is different
      from the usual DTG definition.
     */

    // Create the empty DTG nodes.
    vector<StubbornDTG> dtgs;
    for (VariableProxy var : task_proxy.get_variables()) {
        dtgs.emplace_back(var.get_domain_size());
    }

    // Add DTG arcs.
    for (OperatorProxy op : task_proxy.get_operators()) {
        unordered_map<int, int> preconditions;
        for (FactProxy pre : op.get_preconditions()) {
            preconditions[pre.get_variable().get_id()] = pre.get_value();
        }
        for (EffectProxy effect : op.get_effects()) {
            FactProxy fact = effect.get_fact();
            VariableProxy var = fact.get_variable();
            int var_id = var.get_id();
            int eff_val = fact.get_value();
            int pre_val = utils::get_value_or_default(preconditions, var_id, -1);

            StubbornDTG &dtg = dtgs[var_id];
            if (pre_val == -1) {
                for (int value = 0; value < var.get_domain_size(); ++value) {
                    dtg[value].push_back(eff_val);
                }
            } else {
                dtg[pre_val].push_back(eff_val);
            }
        }
    }
    return dtgs;
}

void recurse_forwards(const StubbornDTG &dtg,
                      int start_value,
                      int current_value,
                      vector<bool> &reachable) {
    if (!reachable[current_value]) {
        reachable[current_value] = true;
        for (int successor_value : dtg[current_value])
            recurse_forwards(dtg, start_value, successor_value, reachable);
    }
}

// Relies on both fact sets being sorted by variable.
void get_conflicting_vars(const vector<FactPair> &facts1,
                          const vector<FactPair> &facts2,
                          vector<int> &conflicting_vars) {
    conflicting_vars.clear();
    auto facts1_it = facts1.begin();
    auto facts2_it = facts2.begin();
    while (facts1_it != facts1.end() &&
           facts2_it != facts2.end()) {
        if (facts1_it->var < facts2_it->var) {
            ++facts1_it;
        } else if (facts1_it->var > facts2_it->var) {
            ++facts2_it;
        } else {
            if (facts2_it->value != facts1_it->value) {
                conflicting_vars.push_back(facts2_it->var);
            }
            ++facts1_it;
            ++facts2_it;
        }
    }
}

void StubbornSetsEC::initialize(const shared_ptr<AbstractTask> &task) {
    StubbornSets::initialize(task);
    TaskProxy task_proxy(*task);
    VariablesProxy variables = task_proxy.get_variables();
    written_vars.assign(variables.size(), false);
    nes_computed.resize(variables.size());
    for (VariableProxy var : variables) {
        nes_computed[var.get_id()].assign(var.get_domain_size(), false);
    }
    OperatorsProxy operators = task_proxy.get_operators();
    active_ops.assign(operators.size(), false);
    compute_operator_preconditions(task_proxy);
    compute_conflicts_and_disabling(operators.size());
    build_reachability_map(task_proxy);
    cout << "pruning method: stubborn sets ec" << endl;
}

void StubbornSetsEC::compute_operator_preconditions(const TaskProxy &task_proxy) {
    OperatorsProxy operators = task_proxy.get_operators();
    int num_variables = task_proxy.get_variables().size();
    op_preconditions_on_var.resize(operators.size());
    for (OperatorProxy op : operators) {
        op_preconditions_on_var[op.get_id()].resize(num_variables, -1);
        for (FactProxy precondition : op.get_preconditions()) {
            int var_id = precondition.get_variable().get_id();
            int value = precondition.get_value();
            op_preconditions_on_var[op.get_id()][var_id] = value;
        }
    }
}

void StubbornSetsEC::build_reachability_map(const TaskProxy &task_proxy) {
    vector<StubbornDTG> dtgs = build_dtgs(task_proxy);
    VariablesProxy variables = task_proxy.get_variables();
    reachability_map.resize(variables.size());
    for (VariableProxy var : variables) {
        int var_id = var.get_id();
        StubbornDTG &dtg = dtgs[var_id];
        int num_values = var.get_domain_size();
        reachability_map[var_id].resize(num_values);
        for (int start_value = 0; start_value < num_values; ++start_value) {
            vector<bool> &reachable = reachability_map[var_id][start_value];
            reachable.assign(num_values, false);
            recurse_forwards(dtg, start_value, start_value, reachable);
        }
    }
}

void StubbornSetsEC::compute_active_operators(const State &state) {
    active_ops.assign(active_ops.size(), false);

    int num_operators = sorted_op_preconditions.size();
    for (int op_no = 0; op_no < num_operators; ++op_no) {
        bool all_preconditions_are_active = true;

        for (const FactPair &precondition : sorted_op_preconditions[op_no]) {
            int var_id = precondition.var;
            int current_value = state[var_id].get_value();
            const vector<bool> &reachable_values =
                reachability_map[var_id][current_value];
            if (!reachable_values[precondition.value]) {
                all_preconditions_are_active = false;
                break;
            }
        }

        if (all_preconditions_are_active) {
            active_ops[op_no] = true;
        }
    }
}

void StubbornSetsEC::compute_conflicts_and_disabling(int num_operators) {
    conflicting_and_disabling.resize(num_operators);
    disabled.resize(num_operators);

    for (int op1_no = 0; op1_no < num_operators; ++op1_no) {
        for (int op2_no = 0; op2_no < num_operators; ++op2_no) {
            if (op1_no != op2_no) {
                bool conflict = can_conflict(op1_no, op2_no);
                bool disable = can_disable(op2_no, op1_no);
                if (conflict || disable) {
                    conflicting_and_disabling[op1_no].push_back(op2_no);
                }
                if (disable) {
                    disabled[op2_no].push_back(op1_no);
                }
            }
        }
    }
}

bool StubbornSetsEC::is_applicable(int op_no, const State &state) {
    return find_unsatisfied_precondition(op_no, state) == FactPair::no_fact;
}

// TODO: find a better name.
void StubbornSetsEC::mark_as_stubborn_and_remember_written_vars(
    int op_no, const State &state) {
    if (mark_as_stubborn(op_no)) {
        if (is_applicable(op_no, state)) {
            for (const FactPair &effect : sorted_op_effects[op_no])
                written_vars[effect.var] = true;
        }
    }
}

/* TODO: think about a better name, which distinguishes this method
   better from the corresponding method for simple stubborn sets */
void StubbornSetsEC::add_nes_for_fact(const FactPair &fact, const State &state) {
    for (int achiever : achievers[fact.var][fact.value]) {
        if (active_ops[achiever]) {
            mark_as_stubborn_and_remember_written_vars(achiever, state);
        }
    }

    nes_computed[fact.var][fact.value] = true;
}

void StubbornSetsEC::add_conflicting_and_disabling(int op_no,
                                                   const State &state) {
    for (int conflict : conflicting_and_disabling[op_no]) {
        if (active_ops[conflict])
            mark_as_stubborn_and_remember_written_vars(conflict, state);
    }
}

// Relies on op_effects and op_preconditions being sorted by variable.
void StubbornSetsEC::get_disabled_vars(
    int op1_no, int op2_no, vector<int> &disabled_vars) {
    get_conflicting_vars(sorted_op_effects[op1_no],
                         sorted_op_preconditions[op2_no],
                         disabled_vars);
}

void StubbornSetsEC::apply_s5(int op_no, const State &state) {
    // Find a violated state variable and check if stubborn contains a writer for this variable.
    for (const FactPair &pre : sorted_op_preconditions[op_no]) {
        if (state[pre.var].get_pair() != pre && written_vars[pre.var]) {
            if (!nes_computed[pre.var][pre.value]) {
                add_nes_for_fact(pre, state);
            }
            return;
        }
    }

    FactPair violated_precondition = find_unsatisfied_precondition(op_no, state);
    assert(violated_precondition != FactPair::no_fact);
    if (!nes_computed[violated_precondition.var][violated_precondition.value]) {
        add_nes_for_fact(violated_precondition, state);
    }
}

void StubbornSetsEC::initialize_stubborn_set(const State &state) {
    for (vector<bool> &by_value : nes_computed) {
        by_value.assign(by_value.size(), false);
    }
    written_vars.assign(written_vars.size(), false);

    compute_active_operators(state);

    //rule S1
    FactPair unsatisfied_goal = find_unsatisfied_goal(state);
    assert(unsatisfied_goal != FactPair::no_fact);
    add_nes_for_fact(unsatisfied_goal, state);     // active operators used
}

void StubbornSetsEC::handle_stubborn_operator(const State &state, int op_no) {
    if (is_applicable(op_no, state)) {
        //Rule S2 & S3
        add_conflicting_and_disabling(op_no, state);     // active operators used
        //Rule S4'
        vector<int> disabled_vars;
        for (int disabled_op_no : disabled[op_no]) {
            if (active_ops[disabled_op_no]) {
                get_disabled_vars(op_no, disabled_op_no, disabled_vars);
                if (!disabled_vars.empty()) {     // == can_disable(op1_no, op2_no)
                    bool v_applicable_op_found = false;
                    for (int disabled_var : disabled_vars) {
                        //First case: add o'
                        if (is_v_applicable(disabled_var,
                                            disabled_op_no,
                                            state,
                                            op_preconditions_on_var)) {
                            mark_as_stubborn_and_remember_written_vars(
                                disabled_op_no, state);
                            v_applicable_op_found = true;
                            break;
                        }
                    }

                    //Second case: add a necessary enabling set for o' following S5
                    if (!v_applicable_op_found) {
                        apply_s5(disabled_op_no, state);
                    }
                }
            }
        }
    } else {     // op is inapplicable
        //S5
        apply_s5(op_no, state);
    }
}

static shared_ptr<PruningMethod> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "StubbornSetsEC",
        "Stubborn sets represent a state pruning method which computes a subset "
        "of applicable operators in each state such that completeness and "
        "optimality of the overall search is preserved. As stubborn sets rely "
        "on several design choices, there are different variants thereof. "
        "The variant 'StubbornSetsEC' resolves the design choices such that "
        "the resulting pruning method is guaranteed to strictly dominate the "
        "Expansion Core pruning method. For details, see" + utils::format_paper_reference(
            {"Martin Wehrle", "Malte Helmert", "Yusra Alkhazraji", "Robert Mattmueller"},
            "The Relative Pruning Power of Strong Stubborn Sets and Expansion Core",
            "http://www.aaai.org/ocs/index.php/ICAPS/ICAPS13/paper/view/6053/6185",
            "Proceedings of the 23rd International Conference on Automated Planning "
            "and Scheduling (ICAPS 2013)",
            "251-259",
            "AAAI Press 2013"));

    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<StubbornSetsEC>();
}

static PluginShared<PruningMethod> _plugin("stubborn_sets_ec", _parse);
}
