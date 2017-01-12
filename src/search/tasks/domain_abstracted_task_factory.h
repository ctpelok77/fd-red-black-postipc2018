#ifndef TASKS_DOMAIN_ABSTRACTED_TASK_FACTORY_H
#define TASKS_DOMAIN_ABSTRACTED_TASK_FACTORY_H

#include "../utils/hash.h"

#include <memory>
#include <vector>

class AbstractTask;


namespace extra_tasks {
using ValueGroup = std::vector<int>;
using ValueGroups = std::vector<ValueGroup>;
using VarToGroups = utils::UnorderedMap<int, ValueGroups>;

/*
  Factory for creating domain abstractions.
*/
std::shared_ptr<AbstractTask> build_domain_abstracted_task(
    const std::shared_ptr<AbstractTask> &parent,
    const VarToGroups &value_groups);
}

#endif
