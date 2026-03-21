/* Jazz (c) 2018-2026 kaalam.ai (The Authors of Jazz), using (under the same license):

	1. Biomodelling - The AATBlockQueue class (c) Jacques Basaldúa, 2009-2012 licensed
	  exclusively for the use in the Jazz server software.

	  Copyright 2009-2012 Jacques Basaldúa

	2. BBVA - Jazz: A lightweight analytical web server for data-driven applications.

      Copyright 2016-2017 Banco Bilbao Vizcaya Argentaria, S.A.

      This product includes software developed at

      BBVA (https://www.bbva.com/)

	3. LMDB, Copyright 2011-2017 Howard Chu, Symas Corp. All rights reserved.

	  Licensed under http://www.OpenLDAP.org/license.html


	  Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	  http://www.apache.org/licenses/LICENSE-2.0

	  Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/


#include "src/jazz_core/opcodes.h"


namespace jazz_core
{

/** \brief A table to locate all the possible ONNX tensor types by their name in the config file "onnx.ini".

	The table is a map of strings to a TensorType: Jazz native type, ONNX protocol buffer type and ONNX runtime type.
*/
TensorTypeDict TENSOR_TYPES = {

	{(pChar) "bool",	{CELL_TYPE_BYTE_BOOLEAN, onnx::TensorProto::BOOL,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL}},
	{(pChar) "double",	{CELL_TYPE_DOUBLE,		 onnx::TensorProto::DOUBLE,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE}},
	{(pChar) "float",	{CELL_TYPE_SINGLE,		 onnx::TensorProto::FLOAT,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT}},
	{(pChar) "string",	{CELL_TYPE_STRING,		 onnx::TensorProto::STRING,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING}},

	{(pChar) "bfloat16",{CELL_TYPE_BFLOAT16,	 onnx::TensorProto::BFLOAT16, ONNX_TENSOR_ELEMENT_DATA_TYPE_BFLOAT16}},
	{(pChar) "float16",	{CELL_TYPE_FLOAT16,		 onnx::TensorProto::FLOAT16,  ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16}},

	{(pChar) "int16",	{CELL_TYPE_INT16,		 onnx::TensorProto::INT16,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_INT16}},
	{(pChar) "int32",	{CELL_TYPE_INTEGER,		 onnx::TensorProto::INT32,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32}},
	{(pChar) "int64",	{CELL_TYPE_LONG_INTEGER, onnx::TensorProto::INT64,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64}},
	{(pChar) "int8",	{CELL_TYPE_INT8,		 onnx::TensorProto::INT8,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_INT8}},

	{(pChar) "uint16",	{CELL_TYPE_UINT16,		 onnx::TensorProto::UINT16,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT16}},
	{(pChar) "uint32",	{CELL_TYPE_UINT32,		 onnx::TensorProto::UINT32,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT32}},
	{(pChar) "uint64",	{CELL_TYPE_UINT64,		 onnx::TensorProto::UINT64,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT64}},
	{(pChar) "uint8",	{CELL_TYPE_BYTE,		 onnx::TensorProto::UINT8,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8}}
};


/** \brief A table to locate all the possible ONNX attribute types by their name in the config file "onnx.ini".

	The table is a map of strings to an AttributeType: Jazz native type, ONNX protobuf type and a boolean if the attribute is a list.
*/
AttributeTypeDict ATTRIBUTE_TYPES = {

	{(pChar) "float",	{CELL_TYPE_SINGLE,		 onnx::AttributeProto::AttributeType::AttributeProto_AttributeType_FLOAT, false}},
	{(pChar) "floats",	{CELL_TYPE_SINGLE,		 onnx::AttributeProto::AttributeType::AttributeProto_AttributeType_FLOATS, true}},
	{(pChar) "int",		{CELL_TYPE_LONG_INTEGER, onnx::AttributeProto::AttributeType::AttributeProto_AttributeType_INT, false}},
	{(pChar) "ints",	{CELL_TYPE_LONG_INTEGER, onnx::AttributeProto::AttributeType::AttributeProto_AttributeType_INTS, true}},
	{(pChar) "string",	{CELL_TYPE_STRING,		 onnx::AttributeProto::AttributeType::AttributeProto_AttributeType_STRING, false}},
	{(pChar) "strings",	{CELL_TYPE_STRING,		 onnx::AttributeProto::AttributeType::AttributeProto_AttributeType_STRING, true}},

	{(pChar) "graph",	{CELL_TYPE_ONNX_GRAPH,	 onnx::AttributeProto::AttributeType::AttributeProto_AttributeType_GRAPH, false}},
	{(pChar) "tensor",	{CELL_TYPE_ONNX_TENSOR,	 onnx::AttributeProto::AttributeType::AttributeProto_AttributeType_TENSOR, false}}
};

/*	-----------------------------------------------
	 OpCodes : I m p l e m e n t a t i o n
--------------------------------------------------- */

/** \brief Bop: Start the OpCodes.

	\param a_logger		A pointer to the logger.
	\param a_config		A pointer to the configuration.
*/
OpCodes::OpCodes(pLogger a_logger, pConfigFile a_config) : Service(a_logger, a_config) {}


StatusCode OpCodes::start() {
	if (!get_conf_key("ONNX_IR_VERSION", ir_vers)) {
		log(log_error_level, "Config key ONNX_IR_VERSION not valid in OpCodes::start");

		return SERVICE_ERROR_BAD_CONFIG;
	}

	String s;

	if (!get_conf_key("ONNX_OPCODE_DEFS_FN", s)) {
		log(log_error_level, "Config key ONNX_OPCODE_DEFS_FN not found in OpCodes::start");

		return SERVICE_ERROR_BAD_CONFIG;
	}

	if (!onnx_conf.load_config(s.c_str())) {
		log_printf(log_error_level, "Failed reading configuration file '%s' in OpCodes::start", s.c_str());

		return SERVICE_ERROR_IO_ERROR;
	}

	if (!build_opcode_dict()) {
		log(log_error_level, "Failed building opcode dictionary in OpCodes::start");

		return SERVICE_ERROR_IO_ERROR;
	}

	return SERVICE_NO_ERROR;
}


StatusCode OpCodes::shut_down() {
	return SERVICE_NO_ERROR;
}


/** \brief Build the opcode dictionary.
	This build the vector opcodes and the dictionary opcodes_idx from the content of onnx_conf.

	\return True if the dictionary was built.
*/
bool OpCodes::build_opcode_dict() {
	String all_name_vers;
	if (!onnx_conf.get_key("__name_vers", all_name_vers))
		return false;

	Name name;
	int vers, n;
	pChar p_ori = (pChar) all_name_vers.c_str();
	pChar p_dst = (pChar) &name;
	pOnnxOpCode op;

	while (true) {
		switch (char c = p_ori++[0]) {
		case 0:
		case ',':
			// Name and version are valid, start reading inputs, outputs and attributes

			op = new OnnxOpCode(name, vers, OnnxParameters(), OnnxParameters(), OnnxAttributes());
			if (op == nullptr)
				return false;

			if (!fill_op_code(*op)) {
				delete op;
				return false;
			}

			opcodes.push_back(*op);
			opcodes_idx[stdNameVersion(name, vers)] = opcodes.size() - 1;

			delete op;

			if (c == 0) {
				op_vers_current = op_vers_latest;

				return fill_all_dict_versions();
			}

			break;

		case '.':
			// Point found. The Name is complete, read the version as an integer.

			if (sscanf(p_ori, "%d%n", &vers, &n) != 1)
				return false;

			p_ori += n;

			p_dst[0] = 0;
			p_dst = (pChar) &name;

			op_vers_latest = std::max(op_vers_latest, vers);

			break;

		default:
			// Continue copying the name

			p_dst++[0] = c;
		}
	}
}


/** \brief Fill an ONNX OpCode.
	This completes all the fields of an OpCode other than name and version. It reads the configuration file onnx_conf and creates
	the necessary objects.

	\param op		The OpCode to fill. (It must have the name and version set). This method fills its inputs, outputs and attributes.

	\return True if the OpCode was filled.
*/
bool OpCodes::fill_op_code(OnnxOpCode &op) {
	char buff[256];
	int n;
	String nam, typ;

	sprintf(buff, "%s.%d.num_inputs", op.name.name, op.opset_version);
	if (!onnx_conf.get_key(buff, n))
		return false;

	for (int i = 0; i < n; i++) {
		sprintf(buff, "%s.%d.input.%d.name", op.name.name, op.opset_version, i);

		if (!onnx_conf.get_key(buff, nam))
			return false;

		sprintf(buff, "%s.%d.input.%d.tensor_types", op.name.name, op.opset_version, i);

		if (!onnx_conf.get_key(buff, typ))
			return false;

		OnnxParameter inp = OnnxParameter((pChar) nam.c_str(), TensorTypes());

		if (!fill_tensor_types(inp.types, typ))
			return false;

		op.inputs.push_back(inp);
	}

	sprintf(buff, "%s.%d.num_outputs", op.name.name, op.opset_version);
	if (!onnx_conf.get_key(buff, n))
		return false;

	for (int i = 0; i < n; i++) {
		sprintf(buff, "%s.%d.output.%d.name", op.name.name, op.opset_version, i);

		if (!onnx_conf.get_key(buff, nam))
			return false;

		sprintf(buff, "%s.%d.output.%d.tensor_types", op.name.name, op.opset_version, i);

		if (!onnx_conf.get_key(buff, typ))
			return false;

		OnnxParameter out = OnnxParameter((pChar) nam.c_str(), TensorTypes());

		if (!fill_tensor_types(out.types, typ))
			return false;

		op.outputs.push_back(out);
	}

	sprintf(buff, "%s.%d.num_attributes", op.name.name, op.opset_version);
	if (!onnx_conf.get_key(buff, n))
		return false;

	for (int i = 0; i < n; i++) {
		sprintf(buff, "%s.%d.attribute.%d.name", op.name.name, op.opset_version, i);

		if (!onnx_conf.get_key(buff, nam))
			return false;

		sprintf(buff, "%s.%d.attribute.%d.type", op.name.name, op.opset_version, i);

		if (!onnx_conf.get_key(buff, typ))
			return false;

		OnnxAttribute attr = OnnxAttribute((pChar) nam.c_str(), AttributeType());

		if (!fill_attribute_type(attr.type, typ))
			return false;

		op.attributes.push_back(attr);
	}

	return true;
}


/** \brief Fill all versions of the dictionary.

	This fills every non existing version of an opcode to the previous existing one. E.g., if and opcode "foo" exists for versions 7 and 13,
	it makes the versions 8 to 12 point to the version 7 opcode and the versions 14 to 19 (latest) point to version 13.

	This simplifies the search for the opcodes for any version predefined using set_opset_version().

	\return True if all versions were filled
*/
bool OpCodes::fill_all_dict_versions() {
	String all_names;
	if (!onnx_conf.get_key("__names", all_names))
		return false;

	std::stringstream ss(all_names);
	String name;

	while (std::getline(ss, name, ',')) {
		int lowest_vers = -1, x;
		char buff[256];

		for (int vers = 1; vers <= op_vers_latest; vers++) {
			sprintf(buff, "%s.%d.num_inputs", name.c_str(), vers);

			if (onnx_conf.get_key(buff, x))
				lowest_vers = vers;
			else if (lowest_vers > 0) {
				opcodes_idx[stdNameVersion((pChar) name.c_str(), vers)] = opcodes_idx[stdNameVersion((pChar) name.c_str(), lowest_vers)];
			}
		}
	}

	return true;
}


/** \brief Fill the tensor types.
	This fills a list of tensor types from a string with the names of the types separated by commas.

	\param types		The list of types to fill.
	\param all_types	The string with the names of the types separated by commas.

	\return True if all types were found.
*/
bool OpCodes::fill_tensor_types(TensorTypes &types, String &all_types) {
	std::stringstream ss(all_types);
	String typ;

	while (std::getline(ss, typ, ',')) {
		TensorTypeDict::iterator it = TENSOR_TYPES.find((pChar) typ.c_str());
		if (it == TENSOR_TYPES.end()) {
			return false;
		}
		types.push_back(it->second);
	}

	return true;
}


/** \brief Fill the attribute type.
	This fills an attribute type from a string with the name of the type.

	\param type			The attribute type to fill.
	\param type_name	The name of the type.

	\return True if the type was found.
*/
bool OpCodes::fill_attribute_type(AttributeType &type, String &type_name) {
	AttributeTypeDict::iterator it = ATTRIBUTE_TYPES.find((pChar) type_name.c_str());
	if (it == ATTRIBUTE_TYPES.end())
		return false;

	type.jazz_type		 = it->second.jazz_type;
	type.onnx_proto_type = it->second.onnx_proto_type;
	type.is_multi		 = it->second.is_multi;

	return true;
}

} // namespace jazz_core

#if defined CATCH_TEST
#include "src/jazz_core/tests/test_opcodes.ctest"
#endif
