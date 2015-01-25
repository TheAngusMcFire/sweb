/**
 * @file PathWalker.cpp
 */

#include "MinixFSTypes.h"
#include "PathWalker.h"

#include "Inode.h"
#include "Dentry.h"
#include "Superblock.h"
#include "FileSystemInfo.h"
#include "string.h"

#define CHAR_DOT '.'
#define NULL_CHAR '\0'
#define CHAR_ROOT '/'
#define SEPARATOR '/'

extern FileSystemInfo* fs_info;

int32 PathWalker::pathWalk ( const char* pathname, uint32 flags_ __attribute__ ((unused)), Dentry*& dentry_, VfsMount*& vfs_mount_ )
{
  // Flag indicating the type of the last path component.
  int32 last_type_ = 0;

  // The last path component
  char* last_ = 0;

  if ( pathname == 0 )
  {
    return PW_ENOTFOUND;
  }

  // check the first character of the path
  if ( *pathname == CHAR_ROOT )
  {
    last_type_ = LAST_ROOT;
    // altroot check

    // start with ROOT
    dentry_ = fs_info->getRoot();
  }
  else
  {
    // start with PWD
    dentry_ = fs_info->getPwd();
  }

  if ( ( dentry_ == 0 ) )
  {
    kprintfd ( "PathWalker: PathWalk> ERROR return not found - dentry: %p\n", dentry_);
    return PW_ENOTFOUND;
  }
  debug ( PATHWALKER, "PathWalk> return success - dentry: %p, vfs_mount: %p\n", dentry_, vfs_mount_ );

  debug ( PATHWALKER,  "pathWalk> pathname : %s\n",pathname );

  debug ( PATHWALKER,  "pathWalk> fs_info->getName() : %s\n", fs_info->getName() );
  if ( pathname == 0 )
  {
    debug ( PATHWALKER, "pathWalk> return pathname not found\n" );
    return PW_ENOTFOUND;
  }

  while ( *pathname == SEPARATOR )
    pathname++;
  if ( !*pathname ) // i.e. path = /
  {
    debug ( PATHWALKER,  "pathWalk> return 0 pathname == \\n\n" );
    return PW_SUCCESS;
  }

  bool parts_left = true;
  while ( parts_left )
  {
    int32 npart_pos = 0;
    int32 npart_len = getNextPartLen ( pathname, npart_pos );
    char npart[npart_len];
    strncpy ( npart, pathname, npart_len );
    npart[npart_len-1] = 0;
    debug ( PATHWALKER,  "pathWalk> npart : %s\n", npart );
    debug ( PATHWALKER,  "pathWalk> npart_pos : %d\n", npart_pos );
    if ( npart_pos < 0 )
    {
      debug ( PATHWALKER, "pathWalk> return path invalid npart_pos < 0 \n" );
      return PW_EINVALID;
    }

    if ( ( *npart == NULL_CHAR ) || ( npart_pos == 0 ) )
    {
      debug ( PATHWALKER,  "pathWalk> return success\n" );
      return PW_SUCCESS;
    }
    pathname += npart_pos;

    last_ = npart;
    if ( *npart == CHAR_DOT )
    {
      if ( * ( npart + 1 ) == NULL_CHAR )
      {
        last_type_ = LAST_DOT;
      }
      else if ( ( * ( npart + 1 ) == CHAR_DOT ) && ( * ( npart + 2 ) == NULL_CHAR ) )
      {
        last_type_ = LAST_DOTDOT;
      }
    }
    else
    {
      last_type_ = LAST_NORM;
    }

    // follow the inode
    // check the VfsMount
    if ( last_type_ == LAST_DOT ) // follow LAST_DOT
    {
      debug ( PATHWALKER,  "pathWalk> follow last dot\n" );
      last_ = 0;
      continue;
    }
    else if ( last_type_ == LAST_DOTDOT ) // follow LAST_DOTDOT
    {
      debug ( PATHWALKER,  "pathWalk> follow last dotdot\n" );
      last_ = 0;

      if ( ( dentry_ == fs_info->getRoot() ) )
      {
        // the dentry_ is the root of file-system
        // because the ROOT has not parent from VfsMount.
        continue;
      }

      Dentry* parent_dentry = dentry_->getParent();
      dentry_ = parent_dentry;
      continue;
    }
    else if ( last_type_ == LAST_NORM ) // follow LAST_NORM
    {
      debug ( PATHWALKER,  "pathWalk> follow last norm last_: %s\n",last_ );
      Inode* current_inode = dentry_->getInode();
      Dentry *found = current_inode->lookup ( last_ );
      if ( found )
        debug ( PATHWALKER,  "pathWalk> found->getName() : %s\n",found->getName() );
      else
        debug ( PATHWALKER,  "pathWalk> no dentry found !!!\n" );
      last_ = 0;
      if ( found != 0 )
      {
        dentry_ = found;
      }
      else
      {
        debug ( PATHWALKER, "pathWalk> return dentry not found\n" );
        return PW_ENOTFOUND;
      }
    }

    while ( *pathname == SEPARATOR )
      pathname++;

    if ( strlen ( pathname ) == 0 )
    {
      break;
    }
  }
  debug ( PATHWALKER,  "pathWalk> return 0 end of function\n" );

  return PW_SUCCESS;
}

int32 PathWalker::getNextPartLen ( const char* path, int32 &npart_len )
{
  char* tmp = 0;
  tmp = strchr ( (char*)path, SEPARATOR );

  npart_len = ( size_t ) ( tmp - path + 1 );

  uint32 length = npart_len;

  if ( tmp == 0 )
  {
    npart_len = strlen ( path );
    length = npart_len + 1;
  }

  return length;
}
