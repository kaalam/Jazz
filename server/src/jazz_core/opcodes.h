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


#include "src/jazz_core/std_wrap.h"
#include "src/onnx_proto/onnx.pb.h"
#include "onnxruntime_c_api.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_BEBOP_OPCODES
#define INCLUDED_JAZZ_BEBOP_OPCODES


/** \brief In Bop-25, opcodes are onnx-runtime operations.
*/
namespace jazz_core
{

/** \brief A type definition for tensors across all the technologies: Jazz native, ONNX protocol buffer and ONNX runtime.
*/
struct TensorType {
	int jazz_type;											///< The Jazz native type.
	onnx::TensorProto::DataType onnx_proto_type;			///< The ONNX protocol buffer type.
	ONNXTensorElementDataType onnx_rt_type;					///< The ONNX runtime type.
};
typedef std::map<stdName, TensorType>	TensorTypeDict;		///< A map of TensorType objects.
typedef std::vector<TensorType>			TensorTypes;		///< A list of TensorType objects.
typedef TensorTypes *pTensorTypes;							///< A pointer to a TensorTypes.


/** \brief A type definition for attributes across Jazz native (with special codes), and ONNX protocol buffer.
*/
struct AttributeType {
	int jazz_type;											///< The Jazz native type.
	onnx::AttributeProto::AttributeType onnx_proto_type;	///< The ONNX protocol buffer type.
	bool is_multi = false;									///< True if the attribute is a list of the type.
};
typedef std::map<stdName, AttributeType> AttributeTypeDict;	///< A map of AttributeType objects.
typedef AttributeType *pAttributeType;						///< A pointer to a AttributeType.


/** \brief A parameter (input or output) for an ONNX OpCode.
*/
class OnnxParameter {

	public:

		/** \brief The constructor for an OnnxParameter.
			\param nam	The name of the parameter.
			\param typ	The types of the parameter.
		*/
		OnnxParameter(stdName nam, TensorTypes typ) {
			name	= nam;
			types	= typ;
		}

		stdName		name;										///< The name of the parameter.
		TensorTypes types;										///< The types of the parameter.
};
typedef std::vector<OnnxParameter>			OnnxParameters;		///< A list of OnnxParameter objects.


/** \brief An attribute for an ONNX OpCode.
*/
class OnnxAttribute {

	public:

		/** \brief The constructor for an OnnxAttribute.
			\param nam	The name of the attribute.
			\param typ	The types of the attribute.
		*/
		OnnxAttribute(stdName nam, AttributeType typ) {
			name = nam;
			type = typ;
		}

		stdName		  name;										///< The name of the attribute.
		AttributeType type;										///< The types of the attribute.
};
typedef std::vector<OnnxAttribute>			OnnxAttributes;		///< A list of OnnxAttribute objects.


/** \brief An ONNX OpCode.
*/
class OnnxOpCode {

	public:

		/** \brief The constructor for an OnnxOpCode.
			\param nam		The name of the ONNX OpCode.
			\param version	The version of the opset.
			\param in		The input parameters.
			\param out		The output parameters.
			\param attr		The attributes.
		*/
		OnnxOpCode(stdName nam, int version, OnnxParameters in, OnnxParameters out, OnnxAttributes attr) {
			name			= nam;
			opset_version	= version;
			inputs			= in;
			outputs			= out;
			attributes		= attr;
		}

		stdName name;							///< The name of the ONNX OpCode.
		int opset_version;						///< The version of the opset.
		OnnxParameters inputs;					///< The input parameters.
		OnnxParameters outputs;					///< The output parameters.
		OnnxAttributes attributes;				///< The attributes.
};
typedef OnnxOpCode *pOnnxOpCode;				///< A pointer to an OnnxOpCode object.
typedef std::vector<OnnxOpCode> OnnxOpCodes;	///< A list of OnnxOpCode objects.


/** \brief A pair of a name and a version to be used as a key in a dictionary.
*/
class stdNameVersion {

	public:

		/** \brief The constructor for a stdNameVersion.
			\param nam		The name of the ONNX OpCode.
			\param version	The version of the opset.
		*/
		stdNameVersion(stdName nam, int version) {
			name		  = nam;
			opset_version = version;
		}

		/** \brief Operator this == o.

			\param o	The object to compare with.

			\return True if the objects are equal.
		*/
		bool operator==(const stdNameVersion &o) const {
			return (opset_version == o.opset_version) && (strcmp(name.name, o.name.name) == 0);
		}

		/** \brief Operator this < o.

			\param o	The object to compare with.

			\return True if this is less than o.
		*/
		bool operator<(const stdNameVersion &o) const {
			return (opset_version < o.opset_version) || ((opset_version == o.opset_version) && (strcmp(name.name, o.name.name) < 0));
		}

		stdName name;				///< The name of the ONNX OpCode.
		int opset_version;			///< The version of the opset.
};
typedef std::map<stdNameVersion, int> OnnxOpCodeDict;		///< A map of OnnxOpCode objects.


/** \brief OpCodes: The opcodes.
*/
class OpCodes : public Service {

	public:

		OpCodes(pLogger a_logger, pConfigFile a_config);

		virtual StatusCode start	();
		virtual StatusCode shut_down();

		/** \brief Get the latest opset version.
			\return The latest opset version.
		*/
		int latest_opset_version() {
			return op_vers_latest;
		}

		/** \brief Set the opset version.
			\param version	The version to set.
			\return True if the version was set. It must be between 1 and latest_opset_version().
		*/
		bool set_opset_version(int version) {
			if (version <= 0 || version > op_vers_latest)
				return false;

			op_vers_current = version;
			return true;
		}

		/** \brief Get an ONNX OpCode.
			\param name	The name of the OpCode.
			\return The OpCode. Null if not found.
				The version returned is highest version smaller or equal to the one set by set_opset_version().
		*/
		pOnnxOpCode get(stdName name) {
			OnnxOpCodeDict::iterator it = opcodes_idx.find(stdNameVersion(name, op_vers_current));

			if (it == opcodes_idx.end())
				return nullptr;

			return &opcodes.at(it->second);
		}

#ifndef CATCH_TEST
	private:
#endif

		bool build_opcode_dict	   ();
		bool fill_op_code		   (OnnxOpCode	  &op);
		bool fill_all_dict_versions();
		bool fill_tensor_types	   (TensorTypes	  &types,
									String	  &all_types);
		bool fill_attribute_type   (AttributeType &type,
									String	  &type_name);

		ConfigFile onnx_conf = ConfigFile(nullptr);		///< The ONNX opcodes reference stored as a ConfigFile.
		int ir_vers = 0;								///< Argument of `model.set_ir_version()` (Stored as ONNX_IR_VERSION in config.)
		int op_vers_latest = 0;							///< The latest opset version in onnx.ini.
		int op_vers_current = 0;						///< The opset version in use, set by set_opset_version() or op_vers_latest.
		OnnxOpCodeDict opcodes_idx = {};				///< The index to the opcodes.
		OnnxOpCodes	opcodes = {};						///< The opcodes parsed as binary objects.
		int log_error_level = LOG_ERROR;				///< The log level to silence it in tests
};
typedef OpCodes *pOpCodes;		///< A pointer to an OpCodes object

} // namespace jazz_core

#endif // ifndef INCLUDED_JAZZ_BEBOP_OPCODES
