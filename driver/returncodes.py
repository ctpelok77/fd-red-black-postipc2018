# -*- coding: utf-8 -*-

from __future__ import print_function

import signal

EXIT_SUCCESS = 0 # translator and search (or only translator if no search is run) successfully completed

EXIT_TRANSLATE_SCRITICAL = 1
EXIT_TRANSLATE_OUT_OF_MEMORY = 100
EXIT_TRANSLATE_SIGXCPU = 256-signal.SIGXCPU if hasattr(signal, "SIGXCPU") else None

EXPECTED_TRANSLATOR_EXITCODES = set([
    EXIT_SUCCESS, EXIT_TRANSLATE_OUT_OF_MEMORY, EXIT_TRANSLATE_SIGXCPU])

EXIT_SEARCH_CRITICAL_ERROR = 1
EXIT_SEARCH_INPUT_ERROR = 2
EXIT_SEARCH_UNSUPPORTED = 3
EXIT_SEARCH_UNSOLVABLE = 4
EXIT_SEARCH_UNSOLVED_INCOMPLETE = 5
EXIT_SEARCH_OUT_OF_MEMORY = 6
EXIT_SEARCH_TIMEOUT = 7
EXIT_SEARCH_TIMEOUT_AND_MEMORY = 8
EXIT_SEARCH_SIGXCPU = -signal.SIGXCPU if hasattr(signal, "SIGXCPU") else None

EXPECTED_SEARCH_EXITCODES = set([
    EXIT_SUCCESS, EXIT_SEARCH_UNSOLVABLE, EXIT_SEARCH_UNSOLVED_INCOMPLETE,
    EXIT_SEARCH_OUT_OF_MEMORY, EXIT_SEARCH_TIMEOUT, EXIT_SEARCH_SIGXCPU])


def generate_portfolio_exitcode(exitcodes):
    """A portfolio's exitcode is determined as follows:

    There is exactly one type of unexpected exit code -> use it.
    There are multiple types of unexpected exit codes -> EXIT_SEARCH_CRITICAL_ERROR.
    [..., EXIT_SUCCESS, ...] -> EXIT_SUCCESS
    [..., EXIT_SEARCH_UNSOLVABLE, ...] -> EXIT_SEARCH_UNSOLVABLE
    [..., EXIT_SEARCH_UNSOLVED_INCOMPLETE, ...] -> EXIT_SEARCH_UNSOLVED_INCOMPLETE
    [..., EXIT_SEARCH_OUT_OF_MEMORY, ..., EXIT_SEARCH_TIMEOUT, ...] -> EXIT_SEARCH_TIMEOUT_AND_MEMORY
    [..., EXIT_SEARCH_TIMEOUT, ...] -> EXIT_SEARCH_TIMEOUT
    [..., EXIT_SEARCH_OUT_OF_MEMORY, ...] -> EXIT_SEARCH_OUT_OF_MEMORY
    """
    print("Exit codes: %s" % exitcodes)
    exitcodes = set(exitcodes)
    if EXIT_SEARCH_SIGXCPU in exitcodes:
        exitcodes.remove(EXIT_SEARCH_SIGXCPU)
        exitcodes.add(EXIT_SEARCH_TIMEOUT)
    unexpected_codes = exitcodes - EXPECTED_SEARCH_EXITCODES
    if unexpected_codes:
        print("Error: Unexpected exit codes: %s" % list(unexpected_codes))
        if len(unexpected_codes) == 1:
            return unexpected_codes.pop()
        else:
            return EXIT_SEARCH_CRITICAL_ERROR
    for code in [EXIT_SUCCESS, EXIT_SEARCH_UNSOLVABLE, EXIT_SEARCH_UNSOLVED_INCOMPLETE]:
        if code in exitcodes:
            return code
    for code in [EXIT_SEARCH_OUT_OF_MEMORY, EXIT_SEARCH_TIMEOUT]:
        if exitcodes == set([code]):
            return code
    if exitcodes == set([EXIT_SEARCH_OUT_OF_MEMORY, EXIT_SEARCH_TIMEOUT]):
        return EXIT_SEARCH_TIMEOUT_AND_MEMORY
    print("Error: Unhandled exit codes: %s" % exitcodes)
    return EXIT_SEARCH_CRITICAL_ERROR
