//
//  Scheduler.hpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#ifndef Scheduler_hpp
#define Scheduler_hpp

#include <vector>
#include <queue>

#include "Interfaces.h"
#include <unordered_map>
#include <set> 

struct CompareMachineEnergy {
    bool operator()(const MachineId_t& a, const MachineId_t& b) const {
        return Machine_GetEnergy(a) > Machine_GetEnergy(b); // min-heap: smaller priority comes first
    }
};

class Scheduler {
public:
    Scheduler()                 {}
    void Init();
    void MigrationComplete(Time_t time, VMId_t vm_id);
    void NewTask(Time_t now, TaskId_t task_id);
    void PeriodicCheck(Time_t now);
    void Shutdown(Time_t now);
    void TaskComplete(Time_t now, TaskId_t task_id);
    void StateChangeComplete(Time_t now, MachineId_t machine_id);
private:
    vector<VMId_t> vms;
    vector<MachineId_t> machines;
    std::unordered_map<VMId_t, MachineId_t> vm_to_machine;
    std::unordered_map<MachineId_t, std::vector<VMId_t>> machine_to_vms;
    std::unordered_map<TaskId_t, VMId_t> task_to_vm;
    std::set<MachineId_t> powered_on;



};



#endif /* Scheduler_hpp */
