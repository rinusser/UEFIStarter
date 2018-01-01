/** \file
 * Timing functions
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_timestamp
 */

#ifndef __TIMESTAMP_H
#define __TIMESTAMP_H

#include <Uefi.h>


double timestamp_diff_seconds(UINT64 start, UINT64 end);
int init_timestamps();
UINT64 get_timestamp();
UINT64 get_timestamp_ticks_per_second();


#endif
