#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from downward.configs import default_configs_optimal
from downward.reports.scatter import ScatterPlotReport

import common_setup


REVS = ["issue214-base", "issue214-v5"]
CONFIGS = {"blind": ["--search", "astar(blind())"]}

TEST_RUN = False

if TEST_RUN:
    SUITE = "gripper:prob01.pddl"
    PRIORITY = None  # "None" means local experiment
else:
    SUITE = list(sorted(set(suites.suite_all()) - set(suites.suite_optimal_with_ipc11())))
    PRIORITY = 0     # number means maia experiment


exp = common_setup.MyExperiment(
    grid_priority=PRIORITY,
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    parsers=['state_size_parser.py'],
    )

exp.add_comparison_table_step(
    attributes=common_setup.MyExperiment.DEFAULT_TABLE_ATTRIBUTES + ['bytes_per_state', 'variables', 'state_var_t_size']
)

exp()
