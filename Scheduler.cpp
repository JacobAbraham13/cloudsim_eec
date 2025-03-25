//
//  Scheduler.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#include "Scheduler.hpp"
#include <climits>


static bool migrating = false;
static unsigned active_machines = 16;

Priority_t determinePriority(SLAType_t sla) {
    switch (sla) {
        case SLA0: return HIGH_PRIORITY;
        case SLA1: return HIGH_PRIORITY;
        case SLA2: return MID_PRIORITY;
        case SLA3: return LOW_PRIORITY;
        default: return MID_PRIORITY;
    }
}

void Scheduler::Init() {
    // Find the parameters of the clusters
    // Get the total number of machines
    // For each machine:
    //      Get the type of the machine
    //      Get the memory of the machine
    //      Get the number of CPUs
    //      Get if there is a GPU or not
    // 
    unsigned total_machines = Machine_GetTotal();
    SimOutput("Scheduler::Init(): Total number of machines is " + to_string(total_machines), 3);
    SimOutput("Scheduler::Init(): Initializing scheduler", 1);

    for (unsigned i = 0; i < active_machines && i < total_machines; i++) {
        machines.push_back(i);
        powered_on.insert(i); // Track that machine is on

        VMId_t vm = VM_Create(LINUX, X86);
        VM_Attach(vm, i);

        vms.push_back(vm);
        vm_to_machine[vm] = i;
        machine_to_vms[i].push_back(vm);
    }

    // Turn off ARM machines (assumed to be machines 24 and up)
    for (unsigned i = 24; i < total_machines; i++) {
        Machine_SetState(i, S5);
    }

    SimOutput("Scheduler::Init(): Initialized " + to_string(active_machines) + " X86 machines with VMs.", 3);

}

void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
    // Update your data structure. The VM now can receive new tasks
}

void Scheduler::NewTask(Time_t now, TaskId_t task_id) {
    TaskInfo_t task_info = GetTaskInfo(task_id);
    Priority_t priority = determinePriority(task_info.required_sla);

    VMId_t best_vm = -1;
    int min_tasks = INT_MAX;

    // STEP 1: Greedily try to assign task to best existing VM
    for (VMId_t vm : vms) {
        VMInfo_t vm_info = VM_GetInfo(vm);
        MachineId_t machine_id = vm_info.machine_id;
        MachineInfo_t m_info = Machine_GetInfo(machine_id);

        if (vm_info.machine_id == (MachineId_t)-1) continue;


        if (m_info.s_state != S0) continue;
        if (vm_info.cpu != task_info.required_cpu || vm_info.vm_type != task_info.required_vm) continue;
        if (IsTaskGPUCapable(task_id) && !m_info.gpus) continue;

        unsigned available_memory = m_info.memory_size - m_info.memory_used;
        if (available_memory < task_info.required_memory + VM_MEMORY_OVERHEAD) continue;

        if ((int)vm_info.active_tasks.size() < min_tasks) {
            best_vm = vm;
            min_tasks = vm_info.active_tasks.size();
        }
    }

    // if (best_vm != -1) {
    //     SimOutput("best_vm machine id" + to_string((VM_GetInfo(best_vm)).machine_id), 3);

    //     VM_AddTask(best_vm, task_id, priority);
    //     SimOutput(" got past iagjfj.", 3);
    //     task_to_vm[task_id] = best_vm;
    //     SimOutput("NewTask(): Assigned to existing VM " + to_string(best_vm), 2);
    //     return;
    // }

    if (best_vm != VMId_t(-1)) {
        VMInfo_t best_vm_info = VM_GetInfo(best_vm);

        // Check VM is attached and not migrating
        // if (best_vm_info.machine_id == -1 || best_vm_info.migrating) {
        //     SimOutput("NewTask(): VM " + to_string(best_vm) + " not ready (migrating or unattached)", 1);
        //     return;
        // }

        VM_AddTask(best_vm, task_id, priority);
        task_to_vm[task_id] = best_vm;
        SimOutput("NewTask(): Assigned to existing VM " + to_string(best_vm), 2);
        return;
    }

    // STEP 2: Try creating a new VM on an already powered-on machine
    for (MachineId_t machine : machines) {
        MachineInfo_t m_info = Machine_GetInfo(machine);
        if (m_info.s_state != S0 || m_info.cpu != task_info.required_cpu) continue;
        if (IsTaskGPUCapable(task_id) && !m_info.gpus) continue;

        unsigned available_memory = m_info.memory_size - m_info.memory_used;
        if (available_memory < task_info.required_memory + VM_MEMORY_OVERHEAD) continue;

        // Create VM and defer task assignment (safe from crash)
        VMId_t new_vm = VM_Create(task_info.required_vm, task_info.required_cpu);
        VM_Attach(new_vm, machine);

        vms.push_back(new_vm);
        vm_to_machine[new_vm] = machine;
        machine_to_vms[machine].push_back(new_vm);

        SimOutput("NewTask(): Created VM " + to_string(new_vm) + " on machine " + to_string(machine) + " — task deferred", 2);
        return;
    }

    // STEP 3: Power on a sleeping machine and defer task until state change
    for (MachineId_t machine : machines) {
        MachineInfo_t m_info = Machine_GetInfo(machine);
        if (m_info.s_state == S5 && m_info.cpu == task_info.required_cpu) {
            Machine_SetState(machine, S0);
            SimOutput("NewTask(): Powered on sleeping machine " + to_string(machine) + " for task " + to_string(task_id), 2);
            return;
        }
    }

    // STEP 4: Fallback
    SimOutput("NewTask(): No placement found for task " + to_string(task_id), 1);
}




// void Scheduler::NewTask(Time_t now, TaskId_t task_id) {
//     // Get the task parameters
//     //  IsGPUCapable(task_id);
//     //  GetMemory(task_id);
//     //  RequiredVMType(task_id);
//     //  RequiredSLA(task_id);
//     //  RequiredCPUType(task_id);
//     // Decide to attach the task to an existing VM, 
//     //      vm.AddTask(taskid, Priority_T priority); or
//     // Create a new VM, attach the VM to a machine
//     //      VM vm(type of the VM)
//     //      vm.Attach(machine_id);
//     //      vm.AddTask(taskid, Priority_t priority) or
//     // Turn on a machine, create a new VM, attach it to the VM, then add the task
//     //
//     // Turn on a machine, migrate an existing VM from a loaded machine....
//     //
//     // Other possibilities as desired
//     // bool foundMachine = false; 


    



//     //Priority_t priority = (task_id == 0 || task_id == 64)? HIGH_PRIORITY : MID_PRIORITY;

//     //loop through all the machines 
//     //see if we can fit it on the machine and if the task can fit on and if the machine is on 

//     bool vm_found = false; 
//     TaskInfo_t task_info = GetTaskInfo(task_id); 
//     Priority_t priority = determinePriority(task_info.required_sla);

//     for(VMId_t vm : vms) {
//         VMInfo_t vm_info = VM_GetInfo(vm);
       

//         MachineId_t machine_id = vm_info.machine_id;
//         MachineInfo_t machine_info = Machine_GetInfo(machine_id);

//         bool isMachineOn = machine_info.s_state == 0;
//         bool isMatchingCPUType = vm_info.cpu == task_info.required_cpu;
//         bool isMatchingVMType = vm_info.vm_type == task_info.required_vm;
//         unsigned available_memory = machine_info.memory_size - machine_info.memory_used; 
//         bool isEnoughMemory = available_memory >= (task_info.required_memory + VM_MEMORY_OVERHEAD);

        
         
//         bool GPU_compatible; 
//         if(IsTaskGPUCapable(task_id)) {
//             // if task needs gpu, then vm needs to have a gp
//             // else machine_info.gpus can be true or false 
//             GPU_compatible = machine_info.gpus; 
//         } else {
//             GPU_compatible = true; 
//         }

//         // if (isMachineOn && isMatchingCPUType && isMatchingVMType && isEnoughMemory && GPU_compatible) {
//         //     vm_found = true;
//         //     VM_AddTask(vm, task_id, priority);
//         //     task_to_vm[task_id] = vm; //? 
//         //     SimOutput("Scheduler::NewTask " + to_string(task_id) + " was detected at time " + to_string(now), 0);
//         //     return; 
//         // } 
//         // if (isMachineOn && isMatchingCPUType && isMatchingVMType && isEnoughMemory) {
//         //     if (priority == HIGH_PRIORITY) {
//         //         VMInfo_t vm_info = VM_GetInfo(vm);
//         //         if (vm_info.active_tasks.size() == 0) {
//         //             // Assign to idle VM immediately
//         //             VM_AddTask(vm, task_id, priority);
//         //             task_to_vm[task_id] = vm;
//         //             SimOutput("NewTask(): SLA0 task " + to_string(task_id) + " assigned to idle VM " + to_string(vm), 2);
//         //             return;
//         //         }
//         //     } else {
//         //         // For MID/LOW priority tasks, allow even busy VMs
//         //         VM_AddTask(vm, task_id, priority);
//         //         task_to_vm[task_id] = vm;
//         //         SimOutput("NewTask(): Task " + to_string(task_id) + " assigned to shared VM " + to_string(vm), 2);
//         //         return;
//         //     }
//         // }

//         if (isMachineOn && isMatchingCPUType && isMatchingVMType && isEnoughMemory && GPU_compatible) {
//             if (priority == HIGH_PRIORITY && vm_info.active_tasks.size() == 0) {
//                 VM_AddTask(vm, task_id, priority);
//                 task_to_vm[task_id] = vm;
//                 return;
//             } else if (priority != HIGH_PRIORITY) {
//                 VM_AddTask(vm, task_id, priority);
//                 task_to_vm[task_id] = vm;
//                 return;
//             }
//         }
      

//     }

//     // No existing VM is suitable for the task, so we will loop through all machines to see where to place 
//     // the new VM and the task
//     // for (MachineId_t machine : machines) {
//     //     MachineInfo_t machine_info = Machine_GetInfo(machine);
//     //     if (machine_info.cpu != task_info.required_cpu) continue;
//     //     unsigned available_memory = machine_info.memory_size - machine_info.memory_used; 
//     //     bool isEnoughMemory = available_memory >= (task_info.required_memory + VM_MEMORY_OVERHEAD);
        
//     //     if (isEnoughMemory && machine_info.cpu == task_info.required_cpu) {
            
//     //         // if (machine_info.s_state != S0) {
//     //         //     Machine_SetState(machine, S0);
//     //         //     powered_on.insert(machine);
//     //         // }
            
//     //         // VMId_t new_vm = VM_Create(task_info.required_vm, task_info.required_cpu);
//     //         // VM_Attach(new_vm, machine);

//     //         // vms.push_back(new_vm);
//     //         // vm_to_machine[new_vm] = machine;
//     //         // machine_to_vms[machine].push_back(new_vm);

//     //         // VM_AddTask(new_vm, task_id, priority);
//     //         // task_to_vm[task_id] = new_vm; 
//     //         // return;
//     //         if (machine_info.s_state == S0) {
//     //             VMId_t new_vm = VM_Create(task_info.required_vm, task_info.required_cpu);
//     //             VM_Attach(new_vm, machine);
            
//     //             vms.push_back(new_vm);
//     //             vm_to_machine[new_vm] = machine;
//     //             machine_to_vms[machine].push_back(new_vm);
//     //             VM_AddTask(new_vm, task_id, priority);
//     //             task_to_vm[task_id] = new_vm;
//     //             return;
//     //         }
            
//     //     }
//     // }
//     for(MachineId_t machine : machines) {
//         MachineInfo_t m_info = Machine_GetInfo(machine);
//         if (m_info.s_state != S0) continue;  // skip if not powered on
//         if (m_info.cpu != task_info.required_cpu) continue;

//         unsigned available_memory = m_info.memory_size - m_info.memory_used;
//         bool isEnoughMemory = available_memory >= (task_info.required_memory + VM_MEMORY_OVERHEAD);
//         // bool gpu_ok = !IsTaskGPUCapable(task_id) || m_info.gpus;

//         if (isEnoughMemory) {
//             VMId_t new_vm = VM_Create(task_info.required_vm, task_info.required_cpu);
//             VM_Attach(new_vm, machine);

//             vms.push_back(new_vm);
//             vm_to_machine[new_vm] = machine;
//             machine_to_vms[machine].push_back(new_vm);

//             VM_AddTask(new_vm, task_id, priority);
//             task_to_vm[task_id] = new_vm;

//             SimOutput("NewTask(): Created new VM " + to_string(new_vm) + " on machine " + to_string(machine) + " for task " + to_string(task_id), 2);
//             return;
//         }
//     }

//     for (MachineId_t machine : machines) {
//         MachineInfo_t m_info = Machine_GetInfo(machine);
//         if (m_info.s_state == S5 && m_info.cpu == task_info.required_cpu) {
//             Machine_SetState(machine, S0);
//             pending_tasks[machine].push_back(task_id);
//             SimOutput("NewTask(): Powered on machine " + to_string(machine) + " for task " + to_string(task_id), 2);
//             return;
//         }
//     }





//     // if !found, create new machine 

//     // if(migrating) {
//     //     VM_AddTask(vms[0], task_id, priority);
//     // }
//     // else {
//     //     VM_AddTask(vms[task_id % active_machines], task_id, priority);
//     //     SimOutput("Scheduler::NewTask " + to_string(task_id) + " was detected at time " + to_string(now), 0);
//     // }
//     // Skeleton code, you need to change it according to your algorithm
// }

void Scheduler::PeriodicCheck(Time_t now) {
    // This method should be called from SchedulerCheck()
    // SchedulerCheck is called periodically by the simulator to allow you to monitor, make decisions, adjustments, etc.
    // Unlike the other invocations of the scheduler, this one doesn't report any specific event
    // Recommendation: Take advantage of this function to do some monitoring and adjustments as necessary
    for (MachineId_t machine : machines) {
        MachineInfo_t machine_info = Machine_GetInfo(machine);
        if (machine_info.active_tasks == 0 && machine_info.active_vms == 0 && powered_on.count(machine)) {
            Machine_SetState(machine, S5);
            powered_on.erase(machine);
        }
    }
}

void Scheduler::Shutdown(Time_t time) {
    // Do your final reporting and bookkeeping here.
    // Report about the total energy consumed
    // Report about the SLA compliance
    // Shutdown everything to be tidy :-)
    for(auto & vm: vms) {
        VM_Shutdown(vm);
    }
    SimOutput("SimulationComplete(): Finished!", 4);
    SimOutput("SimulationComplete(): Time is " + to_string(time), 4);
    SimOutput("Total Energy: " + to_string(Machine_GetClusterEnergy()) + " KW-Hour", 1);
    SimOutput("SLA0: " + to_string(GetSLAReport(SLA0)) + "%", 1);
    SimOutput("SLA1: " + to_string(GetSLAReport(SLA1)) + "%", 1);
    SimOutput("SLA2: " + to_string(GetSLAReport(SLA2)) + "%", 1);
    SimOutput("SLA3: best-effort", 1);
}

void Scheduler::TaskComplete(Time_t now, TaskId_t task_id) {
    // Do any bookkeeping necessary for the data structures
    // Decide if a machine is to be turned off, slowed down, or VMs to be migrated according to your policy
    // This is an opportunity to make any adjustments to optimize performance/energy
    VMId_t vm = task_to_vm[task_id];
    MachineId_t machine = vm_to_machine[vm];
    MachineInfo_t machine_info = Machine_GetInfo(machine);

    if (machine_info.active_tasks == 0 && machine_info.active_vms == 0 && machine_info.s_state == S0) {
        VM_Shutdown(vm);
        Machine_SetState(machine, S5);
        powered_on.erase(machine);
    }
    SimOutput("Scheduler::TaskComplete(): Task " + to_string(task_id) + " is complete at " + to_string(now), 4);
}

// void Scheduler::StateChangeComplete(Time_t time, MachineId_t machine_id) {
//     SimOutput("StateChangeComplete(): Machine " + to_string(machine_id) + " is now ON", 3);


//     for (auto [task_id, vm_id] : pending_vm_tasks[machine_id]) {
//         TaskInfo_t task_info = GetTaskInfo(task_id);
//         Priority_t priority = determinePriority(task_info.required_sla);

//         // VM was already created and attached
//         VM_AddTask(vm_id, task_id, priority);
//         task_to_vm[task_id] = vm_id;

//         SimOutput("StateChangeComplete(): Assigned deferred task " + to_string(task_id) +
//                   " to pre-created VM " + to_string(vm_id), 2);
//     }

// }



// Public interface below

static Scheduler Scheduler;

void InitScheduler() {
    SimOutput("InitScheduler(): Initializing scheduler", 4);
    Scheduler.Init();
}

void HandleNewTask(Time_t time, TaskId_t task_id) {
    SimOutput("HandleNewTask(): Received new task " + to_string(task_id) + " at time " + to_string(time), 4);
    Scheduler.NewTask(time, task_id);
}

void HandleTaskCompletion(Time_t time, TaskId_t task_id) {
    SimOutput("HandleTaskCompletion(): Task " + to_string(task_id) + " completed at time " + to_string(time), 4);
    Scheduler.TaskComplete(time, task_id);
}

void MemoryWarning(Time_t time, MachineId_t machine_id) {
    // The simulator is alerting you that machine identified by machine_id is overcommitted
    SimOutput("MemoryWarning(): Overflow at " + to_string(machine_id) + " was detected at time " + to_string(time), 0);
}

void MigrationDone(Time_t time, VMId_t vm_id) {
    // The function is called on to alert you that migration is complete
    SimOutput("MigrationDone(): Migration of VM " + to_string(vm_id) + " was completed at time " + to_string(time), 4);
    Scheduler.MigrationComplete(time, vm_id);
    migrating = false;
}

void SchedulerCheck(Time_t time) {
    // This function is called periodically by the simulator, no specific event
    SimOutput("SchedulerCheck(): SchedulerCheck() called at " + to_string(time), 4);
    Scheduler.PeriodicCheck(time);
    static unsigned counts = 0;
    counts++;
    if(counts == 10) {
        migrating = true;
        VM_Migrate(1, 9);
    }
}

void SimulationComplete(Time_t time) {
    // This function is called before the simulation terminates Add whatever you feel like.
    cout << "SLA violation report" << endl;
    cout << "SLA0: " << GetSLAReport(SLA0) << "%" << endl;
    cout << "SLA1: " << GetSLAReport(SLA1) << "%" << endl;
    cout << "SLA2: " << GetSLAReport(SLA2) << "%" << endl;     // SLA3 do not have SLA violation issues
    cout << "Total Energy " << Machine_GetClusterEnergy() << "KW-Hour" << endl;
    cout << "Simulation run finished in " << double(time)/1000000 << " seconds" << endl;
    SimOutput("SimulationComplete(): Simulation finished at time " + to_string(time), 4);
    
    Scheduler.Shutdown(time);
}

void SLAWarning(Time_t time, TaskId_t task_id) {
    SetTaskPriority(task_id, HIGH_PRIORITY);
}

void StateChangeComplete(Time_t time, MachineId_t machine_id) {
    // Called in response to an earlier request to change the state of a machine
    
}

