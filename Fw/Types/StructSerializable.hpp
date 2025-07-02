#ifndef STRUCT_SERIALIZABLE_HPP
#define STRUCT_SERIALIZABLE_HPP

#include <Fw/Types/BasicTypes.hpp>
#include <Fw/Types/Serializable.hpp>
#include <cstring>

namespace Fw {

    template <typename T> class StructSerializable : public Fw::Serializable {
    public:

        enum {
            SERIALIZED_SIZE = sizeof(T)
        };

        StructSerializable();
        StructSerializable(const T& val) { this->m_val = val; }
        StructSerializable(const T* val) { this->m_val = *val; }
        const T& operator=(const T& val) { this->m_val = val; return val;}

        bool operator==(const T& val) const {
            return (0 == ::memcmp(
                reinterpret_cast<const void*>(&this->m_val),
                reinterpret_cast<const void*>(&val.m_val),
                sizeof(T)));
        }

        virtual ~StructSerializable() {}

        void set(const T& val) { this->m_val = val; }
        const T& get(void) { return m_val; }

        SerializeStatus serialize(Fw::SerializeBufferBase& buffer) const {
            return buffer.serialize(reinterpret_cast<const U8*>(&this->m_val), sizeof(T), Serialization::OMIT_LENGTH);
        }

        SerializeStatus deserialize(Fw::SerializeBufferBase& buffer) {
            // need to give it a variable to place the length, but we already know it from the type
            FwSizeType len = sizeof(T);
            return buffer.deserialize(reinterpret_cast<U8*>(&this->m_val), len, Serialization::OMIT_LENGTH);
        }
#if FW_SERIALIZABLE_TO_STRING || FW_ENABLE_TEXT_LOGGING || BUILD_UT
        //!< generate text from serializable
        virtual void toString(StringBase& text) const {
            text = typeid(T).name();
        }
#endif

    private:
        T m_val;

    };

}
#endif
