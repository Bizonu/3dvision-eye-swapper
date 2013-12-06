////////////////////////////////////////////////////////////////////////////////////////////////////
///
///  File:        Registry.h
///  Description: Utility class, used to interact with Windows registry.
///  Author:      Chiuta Adrian Marius
///  Created:     23-11-2013
///
///  Licensed under the Apache License, Version 2.0 (the "License");
///  you may not use this file except in compliance with the License.
///  You may obtain a copy of the License at
///  http://www.apache.org/licenses/LICENSE-2.0
///  Unless required by applicable law or agreed to in writing, software
///  distributed under the License is distributed on an "AS IS" BASIS,
///  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
///  See the License for the specific language governing permissions and
///  limitations under the License.
///
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef INCLUDED_REGISTRY_H
#define INCLUDED_REGISTRY_H

#include <windows.h>
#include <tchar.h>
#include <functional>
#include <thread>

namespace Registry
{
    typedef unsigned long long QWORD;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// [FLAGS] The registry access rights flags.
    enum class AccessRights
    {
        None                = 0x00000,
        Query_Value         = 0x00001,  /// Required to query the values of a registry key.
        Set_Value           = 0x00002,  /// Required to create, delete, or set a registry value.
        Create_SubKey       = 0x00004,  /// Required to create a subkey of a registry key.
        Enumerate_SubKeys   = 0x00008,  /// Required to enumerate the subkeys of a registry key.
        Notify              = 0x00010,  /// Required to request change notifications for a registry key or for subkeys of a registry key.
        Create_Link         = 0x00020,  /// Reserved for system use.
        WoW64_64Key         = 0x00100,  /// Indicates that an application on 64-bit Windows should operate on the 64-bit registry view.
        WoW64_32Key         = 0x00200,  /// Indicates that an application on 64-bit Windows should operate on the 32-bit registry view. 
        Write               = 0x20006,  /// Combines the STANDARD_RIGHTS_WRITE, Set_Value, and Create_SubKey access rights.
        Read                = 0x20019,  /// Combines the STANDARD_RIGHTS_READ, Query_Value, Enumerate_SubKeys, and Notify values.
        All_Access          = 0xF003F   /// Combines the STANDARD_RIGHTS_REQUIRED, Query_Value, Set_Value, Create_SubKey, Enumerate_SubKeys, Notify, and Create_Link access rights.
    };

    inline AccessRights operator | (AccessRights a, AccessRights b)
    {
        return static_cast<AccessRights>(static_cast<int>(a) | static_cast<int>(b));
    }

    inline AccessRights operator & (AccessRights a, AccessRights b)
    {
        return static_cast<AccessRights>(static_cast<int>(a) & static_cast<int>(b));
    }

    inline AccessRights operator ^ (AccessRights a, AccessRights b)
    {
        return static_cast<AccessRights>(static_cast<int>(a) ^ static_cast<int>(b));
    }

    inline AccessRights operator ~ (AccessRights a)
    {
        return static_cast<AccessRights>(~static_cast<int>(a));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// [FLAGS] The registry notify flags.
    enum class NotifyEvents
    {
        Change_Name             = 1,    /// Notify the caller if a subkey is added or deleted.
        Change_Attributes       = 2,    /// Notify the caller of changes to the attributes of the key, such as the security descriptor information.
        Change_LastSet          = 4,    /// Notify the caller of changes to a value of the key. This can include adding or deleting a value, or changing an existing value.
        Change_Security         = 8,    /// Notify the caller of changes to the security descriptor of the key.
        All                     = 15
    };

    inline NotifyEvents operator | (NotifyEvents a, NotifyEvents b)
    {
        return static_cast<NotifyEvents>(static_cast<int>(a) | static_cast<int>(b));
    }

    inline NotifyEvents operator & (NotifyEvents a, NotifyEvents b)
    {
        return static_cast<NotifyEvents>(static_cast<int>(a) & static_cast<int>(b));
    }

    inline NotifyEvents operator ^ (NotifyEvents a, NotifyEvents b)
    {
        return static_cast<NotifyEvents>(static_cast<int>(a) ^ static_cast<int>(b));
    }

    inline NotifyEvents operator ~ (NotifyEvents a)
    {
        return static_cast<NotifyEvents>(~static_cast<int>(a));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// The type of the data that can be hold by the registry values
    enum class DataType
    {
        None                        = 0,        // No value type
        String                      = 1,        // Unicode nul terminated string
        Expand_String               = 2,        // Unicode nul terminated string (with environment variable references)
        Binary                      = 3,        // Free form binary
        DWord                       = 4,        // 32-bit number
        DWord_BigEndian             = 5,        // 32-bit number
        Link                        = 6,        // Symbolic Link (unicode)
        Multi_String                = 7,        // Multiple Unicode strings
        Resource_List               = 8,        // Resource list in the resource map
        Full_Resource_Descriptor    = 9,        // Resource list in the hardware description
        Resource_Requirements_List  = 10,
        QWord                       = 11        // 64-bit number
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// The predefined keys for main registry entries.
    enum class PredefinedKey
    {
        Classes_Root,
        Current_User,
        Local_Machine,
        Users,
        Performance_Data,
        Current_Config,
        Performance_Text,
        Performance_NLSText,
        Dyn_Data,
        Current_User_Local_Settings
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    class Value
    {
        friend class Key;
    public:

        const TCHAR*    GetName() const { return m_Name; }
        DataType        GetType() const { return m_Type; }
        DWORD           GetDataSize() const { return m_DataSize; }
        const BYTE*     GetData() const { return m_Data; }
        Key*            GetKey() const { return m_hKey; }

    protected:
        Value(){}
        class Key*  m_hKey;
        DataType    m_Type;
        TCHAR*      m_Name;
        BYTE*       m_Data;
        DWORD       m_DataSize;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Represents a registry subkey that can be manipulated.
    class Key
    {
    public:

        static Key* Open( _In_          PredefinedKey     mainKey,
                          _In_z_        const TCHAR*      subKeyPath,
                          _In_opt_      AccessRights      accessRights  = AccessRights::All_Access,
                          _Out_opt_     LSTATUS*          statusCode     = nullptr );
                             
        static Key* Create( _In_        PredefinedKey     mainKey,
                            _In_z_      const TCHAR*      subKeyPath,
                            _In_opt_    AccessRights      accessRights  = AccessRights::All_Access,
                            _Out_opt_   LSTATUS*          statusCode     = nullptr );

        static bool Exists( _In_        PredefinedKey     mainKey,
                            _In_z_      const TCHAR*      subKeyPath );

        static LSTATUS Delete( _In_     PredefinedKey     mainKey,
                               _In_z_   const TCHAR*      subKeyPath,
                               _In_opt_ AccessRights      accessRights  = AccessRights::None );

        const TCHAR* GetSubKeyPath() const
        {
            return m_subKeyPath;
        }

        PredefinedKey GetMainKey() const
        {
            return m_mainKey;
        }

        AccessRights GetAccessRights() const
        {
            return m_accessRights;
        }

        HKEY GetHKEY() const
        {
            return m_hKey;
        }

        bool KeyWasCreated() const
        {
            return m_hKeyCreated;
        }

        LSTATUS EnumSubKeys() const;

        LSTATUS Flush() const
        {
            return RegFlushKey(m_hKey);
        }

        LSTATUS GetValue(const TCHAR *valueName, void **data, DWORD *dataSize, DataType *dataType ) const;

        LSTATUS GetValueString(const TCHAR *valueName, TCHAR **value ) const;

        LSTATUS GetValueDWORD(const TCHAR *valueName, DWORD *value ) const;

        LSTATUS GetValueQWORD(const TCHAR *valueName, QWORD *value ) const;

        LSTATUS SetValue(const TCHAR *valueName, const void *data, DWORD dataSize, DataType dataType ) const
        {
            return RegSetValueEx( m_hKey, valueName, 0, (DWORD)dataType, (const BYTE*)data, dataSize);
        }

        LSTATUS SetValueString(const TCHAR *valueName, const TCHAR *value ) const
        {
            DWORD strLen = (value != nullptr) ? (DWORD)_tcslen(value) : 0;
            return RegSetValueEx( m_hKey, valueName, 0, REG_SZ, (const BYTE*)value, strLen);
        }

        LSTATUS SetValueDWORD(const TCHAR *valueName, DWORD value ) const
        {
            return RegSetValueEx( m_hKey, valueName, 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));
        }

        LSTATUS SetValueQWORD(const TCHAR *valueName, QWORD value ) const
        {
            return RegSetValueEx( m_hKey, valueName, 0, REG_QWORD, (const BYTE*)&value, sizeof(QWORD));
        }

        LSTATUS DeleteSubkey( _In_z_   const TCHAR* subKeyPath,
                              _In_opt_ AccessRights accessRights  = AccessRights::None ) const;

        LSTATUS DeleteValue( _In_opt_z_ const TCHAR* valueName ) const;

        LSTATUS EnumSubKeys( _In_ const std::function <bool (_In_ Key &)>& callBack ) const;

        LSTATUS EnumValues( _In_ const std::function <bool (_In_ Value &)>& callBack );

        LSTATUS AddNotify( _In_ const std::function <bool (_In_ Key &, _In_opt_ void*)>& callBack,
                           _In_opt_ NotifyEvents events = NotifyEvents::All,
                           _In_opt_ bool watchSubtree = true,
                           _In_opt_ void *userData = nullptr );

        void Close()
        {
            m_workerShouldClose = true;

            if(m_hKey != nullptr)
                RegCloseKey(m_hKey);

            if(m_worker != nullptr)
                m_worker->join();

            delete [] m_subKeyPath;

            delete this;
        }

    private:
        Key()
        {
            m_mainKey           = PredefinedKey::Current_User;
            m_subKeyPath        = nullptr;
            m_accessRights      = AccessRights::None;
            m_hKey              = nullptr;
            m_hKeyCreated       = false;

            m_workerShouldClose = false;
            m_worker            = nullptr;
        };

        Key( _In_       PredefinedKey      mainKey,
             _In_z_     const TCHAR*       subKeyPath,
             _In_       bool               hKeyCreated,
             _In_       AccessRights       accessRights,
             _In_       HKEY               hKey )
        {
            m_mainKey           = mainKey;
            m_subKeyPath        = subKeyPath;
            m_accessRights      = accessRights;
            m_hKey              = hKey;
            m_hKeyCreated       = hKeyCreated;

            m_workerShouldClose = false;
            m_worker            = nullptr;
        }

        static Key* OpenKey(_In_        PredefinedKey     mainKey,
                            _In_z_      const TCHAR*      subKeyPath,
                            _In_        bool              createKey,
                            _In_        AccessRights      accessRights,
                            _Out_       LSTATUS*          statusCode );

        PredefinedKey       m_mainKey;
        const TCHAR*        m_subKeyPath;
        AccessRights        m_accessRights;
        HKEY                m_hKey;
        bool                m_hKeyCreated;

        volatile bool       m_workerShouldClose;
        std::thread*        m_worker;

        static void NotifyWorker( _In_ Key *key,
                                  _In_ const std::function <bool (_In_ Key &, _In_opt_ void*)>& callBack,
                                  _In_opt_ NotifyEvents events = NotifyEvents::All,
                                  _In_opt_ bool watchSubtree = true,
                                  _In_opt_ void* userData = nullptr );

        static const HKEY   m_predefinedKeys[];
    };
}

#endif // INCLUDED_REGISTRY_H
