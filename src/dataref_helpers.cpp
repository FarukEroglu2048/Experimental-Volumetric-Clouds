#include <dataref_helpers.hpp>
std::unordered_map<std::string, std::vector<float>*> vector_float_dataRefs;
std::unordered_map<std::string, float*> float_dataRefs;
#define MSG_ADD_DATAREF 0x01000000           //  Add dataref to DRE message
float read_float_callback(void* data_pointer)
{
	return *static_cast<float*>(data_pointer);
}

void write_float_callback(void* data_pointer, float input_value)
{
	*static_cast<float*>(data_pointer) = input_value;
}

int read_float_vector_callback(void* data_pointer, float* output_values, int read_offset, int read_size)
{
	std::vector<float>& float_vector = *static_cast<std::vector<float>*>(data_pointer);

	if (output_values != nullptr)
	{
		int read_start = read_offset % float_vector.size();
		int read_end = read_start + read_size;

		if (read_end < read_start) read_end = read_start;
		else if (read_end > float_vector.size()) read_end = float_vector.size();

		for (int index = read_start; index < read_end; index++) output_values[index] = float_vector[index];

		return read_end - read_start;
	}
	else return float_vector.size();
}

void write_float_vector_callback(void* data_pointer, float* input_values, int write_offset, int write_size)
{
	std::vector<float>& float_vector = *static_cast<std::vector<float>*>(data_pointer);

	int write_start = write_offset % float_vector.size();
	int write_end = write_start + write_size;

	if (write_end < write_start) write_end = write_start;
	else if (write_end > float_vector.size()) write_end = float_vector.size();

	for (int index = write_start; index < write_end; index++) float_vector[index] = input_values[index];
}

XPLMDataRef export_float_dataref(char* dataref_name, float initial_value)
{
	float* float_pointer = new float(initial_value);
	float_dataRefs[std::string(dataref_name)]=float_pointer;
	return XPLMRegisterDataAccessor(dataref_name, xplmType_Float, 1, nullptr, nullptr, read_float_callback, write_float_callback, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, float_pointer, float_pointer);
}

XPLMDataRef export_float_vector_dataref(char* dataref_name, std::vector<float> initial_values)
{
	std::vector<float>* vector_pointer = new std::vector<float>(initial_values);
	vector_float_dataRefs[std::string(dataref_name)]=vector_pointer;
	return XPLMRegisterDataAccessor(dataref_name, xplmType_FloatArray, 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, read_float_vector_callback, write_float_vector_callback, nullptr, nullptr, vector_pointer, vector_pointer);
}

void notify_datarefs()
{
	for (auto x : vector_float_dataRefs) {
        std::string name=x.first;
		XPLMSendMessageToPlugin(XPLM_NO_PLUGIN_ID , MSG_ADD_DATAREF, (void*)name.c_str());  //tell dref editor about it
	}
	for (auto x : float_dataRefs) {
        std::string name=x.first;
		XPLMSendMessageToPlugin(XPLM_NO_PLUGIN_ID , MSG_ADD_DATAREF, (void*)name.c_str());  //tell dref editor about it
	}
}
void clean_datarefs(){
	for (auto x : vector_float_dataRefs) {
        std::vector<float>* vector_pointer=x.second;
		delete vector_pointer;

	}
	for (auto x : float_dataRefs) {
        float* float_pointer =x.second;
		delete  float_pointer;
	}
}