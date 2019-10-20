/*
 *  ALi advanced security module
 *
 *  This file contains the ALi advanced verification management.
 *
 *  Author:
 *	Zhao Owen <owen.zhao@alitech.com>
 *
 *  Copyright (C) 2011 Zhao Owen <owen.zhao@alitech.com>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2,
 *      as published by the Free Software Foundation.
 */

#include "aliasix_misc.h"

/**
 * Get the path
 */
static u_long inline __aliasix_misc_path_walk(struct dentry *dentry, char *buf)
{
    struct dentry *p_dentry = dentry;
    struct dentry *p_dentry_parent = NULL;
    char *p_path = kzalloc(PATH_MAX, GFP_KERNEL);
    int i_ofs = 0;

    buf[0] = '\0';
    while (NULL != p_dentry)
    {  	
        ALIASIX_MNT("Info, Dentry qstr %s %s %s %s\n", \
                    p_dentry->d_name.name, \
					p_dentry->d_iname, \
					p_path, buf);
        strcpy(p_path, p_dentry->d_name.name);
        if (0 != strcmp(p_path, "/") && (p_dentry != dentry))
            strcat(p_path, "/");
		if ('/' == p_path[strlen(p_path) - 1] && 
			'/' == buf[0])
			p_path[strlen(p_path) - 1] = '\0';
        strcat(p_path, buf);
        strcpy(buf, p_path);

    	if (p_dentry != (p_dentry_parent = p_dentry->d_parent))
    		p_dentry = p_dentry_parent;
    	else
    		break;
    }

	strcpy(p_path, buf);
    while ('/' == p_path[i_ofs + 1]) i_ofs++;
    while (0 != strcmp(p_path, "/") && \
		   '/' == p_path[strlen(p_path) - 1]) 
		p_path[strlen(p_path) - 1] = '\0';
    strcpy(buf, &p_path[i_ofs]);

    ALIASIX_MNT("Info: path name %s\n", buf);
    kfree(p_path);
    return strlen(buf);
}    

/*
 * Get the full path of the given file's mount point
 * \@param file: file structure
 * \@buf: name buffer
 * \@ret: length of the path
 */
#include <../fs/mount.h> 

 
static struct dentry *mnt_mountpoint(struct vfsmount *mnt)
{
	return real_mount(mnt)->mnt_mountpoint;
}

static struct dentry *mnt_parent_mountpoint(struct vfsmount *mnt)
{
	return real_mount(mnt)->mnt_parent->mnt_mountpoint;
}

static struct vfsmount *mnt_parent_mnt(struct vfsmount *mnt)
{
	return &(real_mount(mnt)->mnt_parent->mnt);
}

 
static u_long inline aliasix_misc_mnt_walk(struct vfsmount *mnt, char *buf)
{    
    struct dentry *p_mountpoint = mnt_mountpoint(mnt);
    struct dentry *p_mountroot = mnt->mnt_root;
    struct dentry *p_parent_mountpoint = mnt_parent_mountpoint(mnt);
    struct vfsmount *p_mount = mnt;
	struct vfsmount *p_mountparent = NULL;
    char *p_path = kmalloc(PATH_MAX, GFP_KERNEL);
    char *p_mnt_path = kmalloc(PATH_MAX, GFP_KERNEL);
    int i_ofs = 0;
    
    buf[0] = '\0';
    p_path[0] = '\0';
    p_mnt_path[0] = '\0';

    while (NULL != p_mount)
    {
        __aliasix_misc_path_walk(p_mountpoint, p_path);
        
        ALIASIX_MNT("Info, Mount point %s %s %s %s\n", \
                    p_mountpoint->d_iname, \
                    p_mountpoint->d_name.name, \
					p_path, p_mnt_path);
        
        if (0 != strcmp(p_path, "/") && (p_mount != mnt))
            strcat(p_path, "/");
		if ('/' == p_path[strlen(p_path) - 1] && 
			'/' == p_mnt_path[0])
			p_path[strlen(p_path) - 1] = '\0';
        strcat(p_path, p_mnt_path);
        strcpy(p_mnt_path, p_path);

        if (p_mount != (p_mountparent = mnt_parent_mnt(p_mount)))
    		p_mount = p_mountparent;
    	else
    		break;
        p_mountpoint = mnt_mountpoint(p_mount);
        p_mountroot = p_mount->mnt_root;
    }

    /* Skip the prefix // which may added by virtual rootfs */
    while ('/' == p_mnt_path[i_ofs + 1]) i_ofs++;
    while (0 != strcmp(p_mnt_path, "/") && \
		   '/' == p_mnt_path[strlen(p_mnt_path) - 1]) 
		p_mnt_path[strlen(p_mnt_path) - 1] = '\0';
    strcpy(buf, &p_mnt_path[i_ofs]);

    ALIASIX_MNT("Info: mount point name %s\n", buf);
    kfree(p_mnt_path);
    kfree(p_path);
    return strlen(buf);
}

/*
 * Get the full path of the given file
 * \@param file: file structure
 * \@buf: name buffer
 * \@ret: length of the path
 */
u_long aliasix_misc_path_walk(struct file *file, char *buf)
{
    struct dentry *p_dentry = file->f_path.dentry;
    struct vfsmount *p_mount = file->f_path.mnt;
    char *p_path = kmalloc(PATH_MAX, GFP_KERNEL);
    char *p_d_file = kmalloc(PATH_MAX, GFP_KERNEL);
    int i_ofs = 0;
    int len = 0;

    if (NULL == p_dentry || NULL == p_mount)
        goto out;
    
    __aliasix_misc_path_walk(p_dentry, p_d_file);
    aliasix_misc_mnt_walk(p_mount, p_path);

    buf[0] = '\0';
    strcat(buf, p_path);
    strcat(buf, p_d_file);

    /* Skip the prefix // which may added by virtual rootfs */
    while ('/' == buf[i_ofs + 1]) i_ofs++;
    strcpy(p_path, &buf[i_ofs]);
    strcpy(buf, p_path);

    len = strlen(buf);

    ALIASIX_MNT("Info: file name %s %s %s\n", \
                buf, p_path, p_d_file);

out:
    kfree(p_d_file);
    kfree(p_path);
    return len;
}
