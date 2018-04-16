/*
 * bnstruct.h
 *
 *  Created on: 2017年9月27日
 */

#ifndef INCLUDE_BNSTRUCT_H_
#define INCLUDE_BNSTRUCT_H_

#pragma pack (1)
struct ThreadMsg
{
    int m_id;
    char m_pcMsg[1];
};
#pragma pack ()


#endif /* INCLUDE_BNSTRUCT_H_ */
