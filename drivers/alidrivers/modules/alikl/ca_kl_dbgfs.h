#ifndef __CA_KL_DBGFS_H__
#define __CA_KL_DBGFS_H__

#include "ca_kl_priv.h"

#ifdef CONFIG_DEBUG_FS
void ca_kl_dbgfs_create(struct ca_kl_sub_dev *pdev);
void ca_kl_dbgfs_remove(struct ca_kl_sub_dev *pdev);
int ca_kl_dbgfs_add_session(struct ca_kl_session *sess);
int ca_kl_dbgfs_del_session(struct ca_kl_session *sess);
#else
inline void ca_kl_dbgfs_create(struct ca_kl_sub_dev *pdev) {};
inline void ca_kl_dbgfs_remove(struct ca_kl_sub_dev *pdev) {};
inline int ca_kl_dbgfs_add_session(struct ca_kl_session *sess)
{
	return -ENOSYS;
}
inline int ca_kl_dbgfs_del_session(struct ca_kl_session *sess)
{
	return -ENOSYS;
}
#endif

#endif /*__CA_KL_DBGFS_H__*/

