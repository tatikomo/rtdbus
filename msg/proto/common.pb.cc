// Generated by the protocol buffer compiler.  DO NOT EDIT!

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "common.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace {

const ::google::protobuf::Descriptor* CommonRequest_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  CommonRequest_reflection_ = NULL;

}  // namespace


void protobuf_AssignDesc_common_2eproto() {
  protobuf_AddDesc_common_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "common.proto");
  GOOGLE_CHECK(file != NULL);
  CommonRequest_descriptor_ = file->message_type(0);
  static const int CommonRequest_offsets_[3] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CommonRequest, field_1_int32_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CommonRequest, field_2_string_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CommonRequest, field_3_int32_),
  };
  CommonRequest_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      CommonRequest_descriptor_,
      CommonRequest::default_instance_,
      CommonRequest_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CommonRequest, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CommonRequest, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(CommonRequest));
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_common_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    CommonRequest_descriptor_, &CommonRequest::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_common_2eproto() {
  delete CommonRequest::default_instance_;
  delete CommonRequest_reflection_;
}

void protobuf_AddDesc_common_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\014common.proto\"U\n\rCommonRequest\022\025\n\rfield"
    "_1_int32\030\001 \002(\005\022\026\n\016field_2_string\030\002 \002(\t\022\025"
    "\n\rfield_3_int32\030\003 \002(\005", 101);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "common.proto", &protobuf_RegisterTypes);
  CommonRequest::default_instance_ = new CommonRequest();
  CommonRequest::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_common_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_common_2eproto {
  StaticDescriptorInitializer_common_2eproto() {
    protobuf_AddDesc_common_2eproto();
  }
} static_descriptor_initializer_common_2eproto_;


// ===================================================================

#ifndef _MSC_VER
const int CommonRequest::kField1Int32FieldNumber;
const int CommonRequest::kField2StringFieldNumber;
const int CommonRequest::kField3Int32FieldNumber;
#endif  // !_MSC_VER

CommonRequest::CommonRequest()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void CommonRequest::InitAsDefaultInstance() {
}

CommonRequest::CommonRequest(const CommonRequest& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void CommonRequest::SharedCtor() {
  _cached_size_ = 0;
  field_1_int32_ = 0;
  field_2_string_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
  field_3_int32_ = 0;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

CommonRequest::~CommonRequest() {
  SharedDtor();
}

void CommonRequest::SharedDtor() {
  if (field_2_string_ != &::google::protobuf::internal::kEmptyString) {
    delete field_2_string_;
  }
  if (this != default_instance_) {
  }
}

void CommonRequest::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* CommonRequest::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return CommonRequest_descriptor_;
}

const CommonRequest& CommonRequest::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_common_2eproto();  return *default_instance_;
}

CommonRequest* CommonRequest::default_instance_ = NULL;

CommonRequest* CommonRequest::New() const {
  return new CommonRequest;
}

void CommonRequest::Clear() {
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    field_1_int32_ = 0;
    if (has_field_2_string()) {
      if (field_2_string_ != &::google::protobuf::internal::kEmptyString) {
        field_2_string_->clear();
      }
    }
    field_3_int32_ = 0;
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool CommonRequest::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // required int32 field_1_int32 = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &field_1_int32_)));
          set_has_field_1_int32();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(18)) goto parse_field_2_string;
        break;
      }
      
      // required string field_2_string = 2;
      case 2: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_field_2_string:
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_field_2_string()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8String(
            this->field_2_string().data(), this->field_2_string().length(),
            ::google::protobuf::internal::WireFormat::PARSE);
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(24)) goto parse_field_3_int32;
        break;
      }
      
      // required int32 field_3_int32 = 3;
      case 3: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_field_3_int32:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &field_3_int32_)));
          set_has_field_3_int32();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectAtEnd()) return true;
        break;
      }
      
      default: {
      handle_uninterpreted:
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          return true;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
  return true;
#undef DO_
}

void CommonRequest::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // required int32 field_1_int32 = 1;
  if (has_field_1_int32()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(1, this->field_1_int32(), output);
  }
  
  // required string field_2_string = 2;
  if (has_field_2_string()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->field_2_string().data(), this->field_2_string().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    ::google::protobuf::internal::WireFormatLite::WriteString(
      2, this->field_2_string(), output);
  }
  
  // required int32 field_3_int32 = 3;
  if (has_field_3_int32()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(3, this->field_3_int32(), output);
  }
  
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* CommonRequest::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // required int32 field_1_int32 = 1;
  if (has_field_1_int32()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(1, this->field_1_int32(), target);
  }
  
  // required string field_2_string = 2;
  if (has_field_2_string()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->field_2_string().data(), this->field_2_string().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        2, this->field_2_string(), target);
  }
  
  // required int32 field_3_int32 = 3;
  if (has_field_3_int32()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(3, this->field_3_int32(), target);
  }
  
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int CommonRequest::ByteSize() const {
  int total_size = 0;
  
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // required int32 field_1_int32 = 1;
    if (has_field_1_int32()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->field_1_int32());
    }
    
    // required string field_2_string = 2;
    if (has_field_2_string()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->field_2_string());
    }
    
    // required int32 field_3_int32 = 3;
    if (has_field_3_int32()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->field_3_int32());
    }
    
  }
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void CommonRequest::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const CommonRequest* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const CommonRequest*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void CommonRequest::MergeFrom(const CommonRequest& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_field_1_int32()) {
      set_field_1_int32(from.field_1_int32());
    }
    if (from.has_field_2_string()) {
      set_field_2_string(from.field_2_string());
    }
    if (from.has_field_3_int32()) {
      set_field_3_int32(from.field_3_int32());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void CommonRequest::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void CommonRequest::CopyFrom(const CommonRequest& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool CommonRequest::IsInitialized() const {
  if ((_has_bits_[0] & 0x00000007) != 0x00000007) return false;
  
  return true;
}

void CommonRequest::Swap(CommonRequest* other) {
  if (other != this) {
    std::swap(field_1_int32_, other->field_1_int32_);
    std::swap(field_2_string_, other->field_2_string_);
    std::swap(field_3_int32_, other->field_3_int32_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata CommonRequest::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = CommonRequest_descriptor_;
  metadata.reflection = CommonRequest_reflection_;
  return metadata;
}


// @@protoc_insertion_point(namespace_scope)

// @@protoc_insertion_point(global_scope)