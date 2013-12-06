////////////////////////////////////////////////////////////////////////////////////////////////////
///
///  File:        Registry.cpp
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
#include "./Registry.h"

namespace Registry
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    const HKEY Key::m_predefinedKeys[] =
    {
        HKEY_CLASSES_ROOT,
        HKEY_CURRENT_USER,
        HKEY_LOCAL_MACHINE,
        HKEY_USERS,
        HKEY_PERFORMANCE_DATA,
        HKEY_CURRENT_CONFIG,
        HKEY_DYN_DATA,
        HKEY_CURRENT_USER_LOCAL_SETTINGS,
        HKEY_PERFORMANCE_TEXT,
        HKEY_PERFORMANCE_NLSTEXT
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    Key* Key::Open( _In_      PredefinedKey         mainKey,
                    _In_z_    const TCHAR*          subKeyPath,
                    _In_opt_  AccessRights          accessRights,
                    _Out_opt_ LSTATUS*              statusCode )
    {
        return OpenKey(mainKey, subKeyPath, false, accessRights, statusCode);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    Key* Key::Create( _In_      PredefinedKey       mainKey,
                      _In_z_    const TCHAR*        subKeyPath,
                      _In_opt_  AccessRights        accessRights,
                      _Out_opt_ LSTATUS*            statusCode )
    {
        return OpenKey(mainKey, subKeyPath, true, accessRights, statusCode);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    LSTATUS Key::Delete( _In_      PredefinedKey    mainKey,
                         _In_z_    const TCHAR*     subKeyPath,
                         _In_opt_  AccessRights     accessRights )
    {
        if(subKeyPath == nullptr)
            return ERROR_INVALID_PARAMETER;

        if( accessRights != AccessRights::None &&
            accessRights != (AccessRights::WoW64_32Key | AccessRights::WoW64_64Key) )
            return ERROR_INVALID_PARAMETER;

        return RegDeleteKeyEx( m_predefinedKeys[(int)mainKey],
                               subKeyPath,
                               (REGSAM)accessRights,
                               0 );
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool Key::Exists( _In_      PredefinedKey     mainKey,
                      _In_z_    const TCHAR*      subKeyPath )
    {
        if(subKeyPath == nullptr)
            return false;

        HKEY hKey;
        LSTATUS status = RegOpenKeyEx( m_predefinedKeys[(int)mainKey],
                                       subKeyPath,
                                       0,
                                       (REGSAM)AccessRights::Query_Value,
                                       &hKey );
        if(status == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return true;
        }

        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    Key* Key::OpenKey(_In_      PredefinedKey     mainKey,
                      _In_z_    const TCHAR*      subKeyPath,
                      _In_      bool              createKey,
                      _In_      AccessRights      accessRights,
                      _Out_     LSTATUS*          statusCode )
    {
        if(subKeyPath == nullptr)
        {
            if(statusCode != nullptr)
                *statusCode = ERROR_INVALID_PARAMETER;

            return nullptr;
        }

        HKEY hKey       = nullptr;
        LSTATUS status  = ERROR_SUCCESS;
        bool keyCreated = false;
        
        if(!createKey)
            status = RegOpenKeyEx( m_predefinedKeys[(int)mainKey],
                                   subKeyPath,
                                   0,
                                   (REGSAM)accessRights,
                                   &hKey );
        else
        {
            DWORD disposition;
            status = RegCreateKeyEx( m_predefinedKeys[(int)mainKey],
                                     subKeyPath,
                                     0,
                                     nullptr,
                                     REG_OPTION_NON_VOLATILE,
                                     (REGSAM)accessRights,
                                     nullptr,
                                     &hKey,
                                     &disposition);

            keyCreated = (disposition == REG_CREATED_NEW_KEY);
        }

        if(statusCode != nullptr)
            *statusCode = status;

        if(status != ERROR_SUCCESS)
            return nullptr;

        size_t subKeyPathLen    = _tcslen(subKeyPath) + 1;
        TCHAR *subKeyPathCopy   = new TCHAR[subKeyPathLen];
        _tcscpy_s(subKeyPathCopy, subKeyPathLen, subKeyPath);

        return new Key(mainKey, subKeyPathCopy, keyCreated, accessRights, hKey);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    LSTATUS Key::DeleteSubkey( _In_z_   const TCHAR*      subKeyPath,
                               _In_opt_ AccessRights      accessRights ) const
    {
        if( accessRights != AccessRights::None &&
            accessRights != (AccessRights::WoW64_32Key | AccessRights::WoW64_64Key) )
            return ERROR_INVALID_PARAMETER;

        return RegDeleteKeyEx( m_hKey,
                               subKeyPath,
                               (REGSAM)m_accessRights,
                               0 );
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    LSTATUS Key::DeleteValue( _In_opt_z_ const TCHAR* valueName ) const
    {
        return RegDeleteValue( m_hKey,
                               valueName );
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    LSTATUS Key::GetValue(const TCHAR *valueName, void **data, DWORD *dataSize, DataType *dataType ) const
    {
        LSTATUS status = ERROR_SUCCESS;

        if(data == nullptr && dataSize == nullptr && dataType == nullptr)
            return ERROR_INVALID_PARAMETER;

        DWORD   type_;
        void*   data_ = nullptr;
        DWORD   dataSize_;

        status = RegGetValue(m_hKey, nullptr, valueName, RRF_RT_ANY, &type_, data_, &dataSize_);
        if(status != ERROR_SUCCESS)
            return status;

        if(dataType != nullptr)
            *dataType = (DataType)type_;

        if(dataSize != nullptr)
            *dataSize = dataSize_;

        if(data != nullptr)
        {
            dataSize_ += 2;
            data_ = new BYTE[dataSize_];
            status = RegGetValue(m_hKey, nullptr, valueName, RRF_RT_ANY, &type_, data_, &dataSize_);
            if(status != ERROR_SUCCESS)
            {
                *data = nullptr;
                delete [] data_;
                return status;
            }

            *data = data_;
        }

        return ERROR_SUCCESS;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    LSTATUS Key::GetValueString(const TCHAR *valueName, TCHAR **value ) const
    {
        LSTATUS status = ERROR_SUCCESS;

        if(value == nullptr)
            return ERROR_INVALID_PARAMETER;

        TCHAR*  data_ = nullptr;
        DWORD   dataSize_;

        status = RegGetValue(m_hKey, nullptr, valueName, RRF_RT_REG_SZ | RRF_RT_REG_MULTI_SZ | RRF_RT_REG_EXPAND_SZ, nullptr, data_, &dataSize_);
        if(status != ERROR_SUCCESS)
            return status;

        dataSize_ += 2;
        data_ = new TCHAR[dataSize_];
        status = RegGetValue(m_hKey, nullptr, valueName, RRF_RT_REG_SZ | RRF_RT_REG_MULTI_SZ | RRF_RT_REG_EXPAND_SZ, nullptr, data_, &dataSize_);
        if(status != ERROR_SUCCESS)
        {
            *value = nullptr;
            delete [] data_;
            return status;
        }

        *value = data_;

        return ERROR_SUCCESS;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    LSTATUS Key::GetValueDWORD(const TCHAR *valueName, DWORD *value ) const
    {
        LSTATUS status = ERROR_SUCCESS;

        if(value == nullptr)
            return ERROR_INVALID_PARAMETER;

        DWORD   data_;
        DWORD   dataSize_ = sizeof(DWORD);
        status = RegGetValue(m_hKey, nullptr, valueName, RRF_RT_DWORD, nullptr, &data_, &dataSize_);
        if(status != ERROR_SUCCESS)
            return status;

        *value = data_;

        return ERROR_SUCCESS;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    LSTATUS Key::GetValueQWORD(const TCHAR *valueName, QWORD *value ) const
    {
        LSTATUS status = ERROR_SUCCESS;

        if(value == nullptr)
            return ERROR_INVALID_PARAMETER;

        QWORD   data_;
        DWORD   dataSize_ = sizeof(QWORD);
        status = RegGetValue(m_hKey, nullptr, valueName, RRF_RT_QWORD, nullptr, &data_, &dataSize_);
        if(status != ERROR_SUCCESS)
            return status;

        *value = data_;

        return ERROR_SUCCESS;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    LSTATUS Key::EnumValues( _In_ const std::function <bool (_In_ Value &)>& callBack)
    {
        LSTATUS status = ERROR_SUCCESS;

        Value   val;
        DWORD   index = 0;
        TCHAR   *name;
        DWORD   nameLen;
        DWORD   type;
        BYTE    *data;
        DWORD   dataSize;

        DWORD   numValues;
        DWORD   maxValueNameLen;
        DWORD   maxValueDataSize;
        status = RegQueryInfoKey( m_hKey,
                                  nullptr,
                                  nullptr,
                                  nullptr,
                                  nullptr,
                                  nullptr,
                                  nullptr,
                                  &numValues,
                                  &maxValueNameLen,
                                  &maxValueDataSize,
                                  nullptr,
                                  nullptr );

        maxValueNameLen += 2;
        name = new TCHAR[maxValueNameLen];
        data = new BYTE[maxValueDataSize];

        do
        {
            memset(name, 0, sizeof(TCHAR) * maxValueNameLen);
            memset(data, 0, sizeof(BYTE)  * maxValueDataSize);
            nameLen     = maxValueNameLen;
            dataSize    = maxValueDataSize;

            status = RegEnumValue( m_hKey,
                                   index,
                                   name,
                                   &nameLen,
                                   nullptr,
                                   &type,
                                   nullptr,
                                   &dataSize );

            if(status == ERROR_SUCCESS)
            {
                val.m_hKey         = this;
                val.m_Name         = name;
                val.m_Type         = (DataType)type;
                val.m_Data         = data;
                val.m_DataSize     = dataSize;

                if( !callBack(val) )
                    status = ERROR_NO_MORE_ITEMS;
            }

            index++;
        } while (status == ERROR_SUCCESS);

        delete [] name;
        delete [] data;
        
        return (status == ERROR_NO_MORE_ITEMS) ? ERROR_SUCCESS : status;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    LSTATUS Key::EnumSubKeys( _In_ const std::function <bool (_In_ Key &)>& callBack) const
    {
        LSTATUS status = ERROR_SUCCESS;

        Key     key;
        DWORD   index = 0;
        TCHAR   *name;
        DWORD   nameLen;

        DWORD   numKeys;
        DWORD   maxKeyNameLen;
        status = RegQueryInfoKey( m_hKey,
                                  nullptr,
                                  nullptr,
                                  nullptr,
                                  &numKeys,
                                  &maxKeyNameLen,
                                  nullptr,
                                  nullptr,
                                  nullptr,
                                  nullptr,
                                  nullptr,
                                  nullptr );

        maxKeyNameLen += 2;
        name = new TCHAR[maxKeyNameLen];

        do
        {
            memset(name, 0, sizeof(TCHAR) * maxKeyNameLen);
            nameLen     = maxKeyNameLen;

            status = RegEnumKeyEx( m_hKey,
                                   index,
                                   name,
                                   &nameLen,
                                   nullptr,
                                   nullptr,
                                   nullptr,
                                   nullptr );

            if(status == ERROR_SUCCESS)
            {
                key.m_hKey         = this->m_hKey;
                key.m_mainKey      = this->m_mainKey;
                key.m_subKeyPath   = name;
                key.m_accessRights = this->m_accessRights;
                key.m_hKeyCreated  = false;

                if( !callBack(key) )
                    status = ERROR_NO_MORE_ITEMS;
            }

            index++;
        } while (status == ERROR_SUCCESS);

        delete [] name;
        
        return (status == ERROR_NO_MORE_ITEMS) ? ERROR_SUCCESS : status;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    LSTATUS Key::AddNotify( _In_ const std::function <bool (_In_ Key &, _In_opt_ void*)>& callBack,
                            _In_opt_ NotifyEvents events,
                            _In_opt_ bool watchSubtree,
                            _In_opt_ void *userData )
    {
        LSTATUS status = ERROR_SUCCESS;

        if(m_worker != nullptr)
        {
            m_workerShouldClose = true;

            if(m_hKey != nullptr)
            {
                RegCloseKey(m_hKey);
                m_hKey = nullptr;
            }

            if(m_worker != nullptr)
            {
                m_worker->join();
                delete m_worker;
                m_worker = nullptr;
            }

            status = RegOpenKeyEx( m_predefinedKeys[(int)m_mainKey],
                                   m_subKeyPath,
                                   0,
                                   (REGSAM)m_accessRights,
                                   &m_hKey );

            if(status != ERROR_SUCCESS)
                return status;
        }

        m_workerShouldClose = false;
        m_worker = new std::thread(NotifyWorker, this, callBack, events, watchSubtree, userData);

        return status;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void Key::NotifyWorker( _In_ Key *key,
                            _In_ const std::function <bool (_In_ Key &, _In_opt_ void*)>& callBack,
                            _In_opt_ NotifyEvents events,
                            _In_opt_ bool watchSubtree,
                            _In_opt_ void* userData )
    {
        LSTATUS status = ERROR_SUCCESS;

        while (!key->m_workerShouldClose)
        {
            status = RegNotifyChangeKeyValue( key->m_hKey, 
                                              watchSubtree, 
                                              (DWORD)events, 
                                              nullptr, 
                                              FALSE );

            if(!key->m_workerShouldClose)
            {
                if(!callBack(*key, userData))
                {
                    if(key->m_hKey != nullptr)
                    {
                        RegCloseKey(key->m_hKey);
                        key->m_hKey = nullptr;
                    }

                    status = RegOpenKeyEx( m_predefinedKeys[(int)key->m_mainKey],
                                           key->m_subKeyPath,
                                           0,
                                           (REGSAM)key->m_accessRights,
                                           &key->m_hKey );

                    key->m_workerShouldClose = true;

                    return;
                }
            }
        }
    }
}
