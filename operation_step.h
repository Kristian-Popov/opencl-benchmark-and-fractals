#pragma once

#include <iterator>

class OperationIdList
{
public:
#define OPERATION_ID_LIST_VALUES { \
        CopyInputDataToDevice, \
        Calculate, \
        CopyOutputDataFromDevice \
        }
    enum OperationIdEnum
        OPERATION_ID_LIST_VALUES;
    
    static std::vector<OperationIdEnum> Build()
    {
        // TODO optimize?
        //return std::vector<OperationIdEnum>( std::cbegin(OperationIdList::ValueList ), std::cend( OperationIdList::ValueList ) );
        return std::vector<OperationIdEnum>( OPERATION_ID_LIST_VALUES );
    }
private:
    //static OperationIdEnum ValueList[];
};
typedef OperationIdList::OperationIdEnum OperationId;

//OperationId OperationIdList::ValueList[] = OPERATION_ID_LIST_VALUES;

#undef OPERATION_ID_LIST_VALUES