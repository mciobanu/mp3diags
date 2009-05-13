/***************************************************************************
 *   MP3 Diags - diagnosis, repairs and tag editing for MP3 files          *
 *                                                                         *
 *   Copyright (C) 2009 by Marian Ciobanu                                  *
 *   ciobi@inbox.com                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef SerSupportH
#define SerSupportH

//#include  <boost/archive/binary_oarchive.hpp>
//#include  <boost/archive/binary_iarchive.hpp>
#include  <boost/archive/text_oarchive.hpp>
#include  <boost/archive/text_iarchive.hpp>
#include  <boost/serialization/serialization.hpp> // !!! the archive headers must be included before serialization; otherwise compiler errors are triggered; this looks like a Boost 1.33 bug

/*
#include  <boost/archive/text_oarchive.hpp>
#include  <boost/archive/text_iarchive.hpp>
#include  <boost/archive/binary_oarchive.hpp>
#include  <boost/archive/binary_iarchive.hpp>
#include  <boost/serialization/serialization.hpp>
#include  <boost/serialization/vector.hpp>
*/


#endif
