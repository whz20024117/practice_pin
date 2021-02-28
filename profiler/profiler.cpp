#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <map>

#include "pin.H"


LEVEL_BASE::KNOB<LEVEL_BASE::BOOL> profileCalls(KNOB_MODE_WRITEONCE, "pintool", "c", "0", "Profile Calls");

uint64_t call_count = 0;
uint64_t inst_count = 0;
uint64_t syscall_count = 0;
uint64_t cflow_count = 0;

std::map<std::pair<ADDRINT, ADDRINT>, uint64_t> cflow_map;
std::map<std::pair<ADDRINT, ADDRINT>, uint64_t> calls_map;
std::map<ADDRINT, uint64_t> syscalls_map;    // Syscall number store at RAX
std::map<ADDRINT, std::string> func_name;

static void found_call(ADDRINT src, ADDRINT target)
{
    call_count++;
    auto edge = std::pair<ADDRINT, ADDRINT>(src, target);
    calls_map[edge]++;
}

static void found_inst_in_bb(UINT32 n)
{
    inst_count += n;
}

// static void found_syscall(UINT32 n)
// {
//     syscall_count += n;
// }

static void found_cflow(ADDRINT src, ADDRINT target)
{
    cflow_count++;
    auto edge = std::pair<ADDRINT, ADDRINT>(src, target);
    cflow_map[edge]++;
}


static void instrument_bb(BBL bb)
{
    BBL_InsertCall(bb, IPOINT_ANYWHERE, (LEVEL_BASE::AFUNPTR) found_inst_in_bb, 
                    IARG_UINT32, BBL_NumIns(bb),
                    IARG_END);
}

static void instrument_trace(TRACE trace, void *v)
{
    IMG img = IMG_FindByAddress(TRACE_Address(trace));

    if (!IMG_Valid(img) || !IMG_IsMainExecutable(img))
        return;

    for (BBL bb = TRACE_BblHead(trace); BBL_Valid(bb); bb = BBL_Next(bb))
    {
        instrument_bb(bb);
    }
}

static void instrument_insn(INS ins, void *v)
{
    if (!INS_IsValidForIpointTakenBranch(ins))
        return;

    IMG img = IMG_FindByAddress(INS_Address(ins));

    if (!IMG_Valid(img) || !IMG_IsMainExecutable(img))
        return;

    INS_InsertPredicatedCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR) found_cflow, 
                            IARG_INST_PTR,
                            IARG_BRANCH_TARGET_ADDR,
                            IARG_END);

    if (INS_HasFallThrough(ins))
    {
        INS_InsertPredicatedCall(ins, IPOINT_AFTER, (AFUNPTR) found_cflow, 
                            IARG_INST_PTR,
                            IARG_FALLTHROUGH_ADDR,
                            IARG_END);
    }

    if (INS_IsCall(ins))
    {
        if (profileCalls.Value())
        {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) found_call,
                            IARG_INST_PTR,
                            IARG_BRANCH_TARGET_ADDR,
                            IARG_END);
        }
    }
}


static void instrument_img(IMG img, void* v)
{
    if (!IMG_Valid(img) || !IMG_IsMainExecutable(img))
        return;

    for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
    {
        for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
        {
            func_name[RTN_Address(rtn)] = RTN_Name(rtn);
        }
    }
}


static void fini_func(INT32 code, void *v)
{
    FILE *f = fopen("./result.txt", "w");

    fprintf(f, "Section symbols: \n");
    for (auto it = func_name.begin(); it != func_name.end(); it++)
    {
        fprintf(f, "\t %s at %08jx \n", it->second.c_str(), it->first);
    }

    fprintf(f, "\n\nNumber of Executed instructions: %lu \n", inst_count);

}


int main(int argc, char** argv)
{
    PIN_InitSymbols();
    
    if(PIN_Init(argc, argv))
    {
        return 1;
    }

    IMG_AddInstrumentFunction(instrument_img, nullptr);
    TRACE_AddInstrumentFunction(instrument_trace, nullptr);
    INS_AddInstrumentFunction(instrument_insn, nullptr);

    PIN_AddFiniFunction(fini_func, nullptr);

    PIN_StartProgram();

    return 0;

}