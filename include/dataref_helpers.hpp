#pragma once

#include <XPLMDataAccess.h>
#include <XPLMPlugin.h>
#include <vector>
#include <unordered_map>
XPLMDataRef export_float_dataref(char* dataref_name, float initial_value);
XPLMDataRef export_float_vector_dataref(char* dataref_name, std::vector<float> initial_values);
void notify_datarefs();
void clean_datarefs();
//std::unordered_map<std::string, float*> floatdataRefs; //could just use float vector below, but meh
