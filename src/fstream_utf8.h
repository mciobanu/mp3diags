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


#ifndef FStreamUtf8H
#define FStreamUtf8H

/*

Drop-in replacements for ifstream, ofstream, and fstream which takes UTF-8 strings for filenames instead of the system codepage.

Currently it assumes that outside Windows everything is UTF-8, so in those cases it just provides typedefs to the standard classes.

*/


#ifndef WIN32

//inline const char* convertUtf8(const char* szUtf8Name) { return szUtf8Name; }
//int convertUtf8(const char* szUtf8Name, std::ios_base::openmode __mode);
#include <fstream>
typedef std::basic_ifstream<char> ifstream_utf8;
typedef std::basic_ofstream<char> ofstream_utf8;
typedef std::basic_fstream<char> fstream_utf8;

#else

#include  <ext/stdio_filebuf.h>
#include  <istream>
#include  <ostream>

#include  <fcntl.h> // for open()

int convertUtf8(const char* szUtf8Name, std::ios_base::openmode __mode);
#include  <string>
std::wstring wstrFromUtf8(const std::string& s);


//********************************************************************************************
//********************************************************************************************
//*****************                                                          *****************
//***************** Adapted from fstream included in the GNU ISO C++ Library *****************
//*****************                                                          *****************
//***************** From the Free Software Foundation, Inc.                  *****************
//*****************                                                          *****************
//***************** http://gcc.gnu.org/libstdc++/                            *****************
//*****************                                                          *****************
//********************************************************************************************
//********************************************************************************************


template<typename _CharT, typename _Traits = std::char_traits<_CharT> >
class stdio_filebuf_utf8 : public __gnu_cxx::stdio_filebuf<_CharT, _Traits>
{
public:
    stdio_filebuf_utf8() : __gnu_cxx::stdio_filebuf<_CharT, _Traits>() {}

    stdio_filebuf_utf8(int __fd, std::ios_base::openmode __mode, size_t __size = static_cast<size_t>(BUFSIZ)) : __gnu_cxx::stdio_filebuf<_CharT, _Traits>(__fd, __mode, __size) {}

    /*override*/ ~stdio_filebuf_utf8() {}
/*
    typedef _Traits				        traits_type;
    typedef typename traits_type::off_type		off_type;
*/

    typedef _CharT                     	                char_type;
    typedef _Traits                    	                traits_type;
    typedef typename traits_type::int_type 		int_type;
    typedef typename traits_type::pos_type 		pos_type;
    typedef typename traits_type::off_type 		off_type;

    typedef std::basic_streambuf<char_type, traits_type>                        __streambuf_type;
    typedef typename __gnu_cxx::stdio_filebuf<_CharT, _Traits>::__filebuf_type  __filebuf_type;
    typedef typename __gnu_cxx::stdio_filebuf<_CharT, _Traits>::__file_type     __file_type;
    typedef typename traits_type::state_type                                    __state_type;
    typedef typename __gnu_cxx::stdio_filebuf<_CharT, _Traits>::__codecvt_type  __codecvt_type;

    using __gnu_cxx::stdio_filebuf<_CharT, _Traits>::open;

    __filebuf_type* open(int __fd, std::ios_base::openmode __mode)
    {
        //close();
        __filebuf_type *__ret = NULL;
        if (!this->is_open())
        {
            this->_M_file.sys_open(__fd, __mode);
            if (this->is_open())
            {
                this->_M_allocate_internal_buffer();
                this->_M_mode = __mode;

                // Setup initial buffer to 'uncommitted' mode.
                this->_M_reading = false;
                this->_M_writing = false;
                this->_M_set_buffer(-1);

                // Reset to initial state.
                this->_M_state_last = this->_M_state_cur = this->_M_state_beg;

                // 27.8.1.3,4
                if ((__mode & std::ios_base::ate)
                    && this->seekoff(0, std::ios_base::end, __mode)
                    == pos_type(off_type(-1)))
                    this->close();
                else
                    __ret = this;

//                this->_M_mode = __mode;
                //this->_M_buf_size = __size;
//                this->_M_allocate_internal_buffer();
//                this->_M_reading = false;
//                this->_M_writing = false;
//                this->_M_set_buffer(-1);
            }
        }
        return __ret;
    }


/*
    stdio_filebuf(int __fd, std::ios_base::openmode __mode, size_t __size)
    {
        if (this->is_open())
        {
            this->_M_mode = __mode;
            this->_M_buf_size = __size;
            this->_M_allocate_internal_buffer();
            this->_M_reading = false;
            this->_M_writing = false;
            this->_M_set_buffer(-1);
        }
    }
*/
};



  // [27.8.1.5] Template class basic_ifstream
  /**
   *  @brief  Controlling input for files.
   *
   *  This class supports reading from named files, using the inherited
   *  functions from std::basic_istream.  To control the associated
   *  sequence, an instance of std::basic_filebuf is used, which this page
   *  refers to as @c sb.
  */
  template<typename _CharT, typename _Traits = std::char_traits<_CharT> >
    class basic_ifstream_utf8 : public std::basic_istream<_CharT, _Traits>
    {
    public:
      // Types:
      typedef _CharT 					char_type;
      typedef _Traits 					traits_type;
      typedef typename traits_type::int_type 		int_type;
      typedef typename traits_type::pos_type 		pos_type;
      typedef typename traits_type::off_type 		off_type;

      // Non-standard types:
      typedef stdio_filebuf_utf8<char_type, traits_type> 	__filebuf_type;
      typedef std::basic_istream<char_type, traits_type>	__istream_type;

    private:
      __filebuf_type	_M_filebuf;

    public:
      // Constructors/Destructors:
      /**
       *  @brief  Default constructor.
       *
       *  Initializes @c sb using its default constructor, and passes
       *  @c &sb to the base class initializer.  Does not open any files
       *  (you haven't given it a filename to open).
      */
      basic_ifstream_utf8() : __istream_type(), _M_filebuf()
      { this->init(&_M_filebuf); }

      /**
       *  @brief  Create an input file stream.
       *  @param  s  Null terminated string specifying the filename.
       *  @param  mode  Open file in specified mode (see std::ios_base).
       *
       *  @c ios_base::in is automatically included in @a mode.
       *
       *  Tip:  When using std::string to hold the filename, you must use
       *  .c_str() before passing it to this constructor.
      */
      explicit
      basic_ifstream_utf8(const char* __s, std::ios_base::openmode __mode = std::ios_base::in)
      : __istream_type(), _M_filebuf()
      {
	this->init(&_M_filebuf);
	this->open(__s, __mode);
      }

      /**
       *  @brief  The destructor does nothing.
       *
       *  The file is closed by the filebuf object, not the formatting
       *  stream.
      */
      ~basic_ifstream_utf8()
      { }

      // Members:
      /**
       *  @brief  Accessing the underlying buffer.
       *  @return  The current basic_filebuf buffer.
       *
       *  This hides both signatures of std::basic_ios::rdbuf().
      */
      __filebuf_type*
      rdbuf() const
      { return const_cast<__filebuf_type*>(&_M_filebuf); }

      /**
       *  @brief  Wrapper to test for an open file.
       *  @return  @c rdbuf()->is_open()
      */
      bool
      is_open()
      { return _M_filebuf.is_open(); }

      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 365. Lack of const-qualification in clause 27
      bool
      is_open() const
      { return _M_filebuf.is_open(); }

      /**
       *  @brief  Opens an external file.
       *  @param  s  The name of the file.
       *  @param  mode  The open mode flags.
       *
       *  Calls @c std::basic_filebuf::open(s,mode|in).  If that function
       *  fails, @c failbit is set in the stream's error state.
       *
       *  Tip:  When using std::string to hold the filename, you must use
       *  .c_str() before passing it to this constructor.
      */
      void
      open(const char* __s, std::ios_base::openmode __mode = std::ios_base::in)
      {
	if (!_M_filebuf.open(convertUtf8(__s, __mode | std::ios_base::in), __mode | std::ios_base::in))
	  this->setstate(std::ios_base::failbit);
	else
	  // _GLIBCXX_RESOLVE_LIB_DEFECTS
	  // 409. Closing an fstream should clear error state
	  this->clear();
      }

      /**
       *  @brief  Close the file.
       *
       *  Calls @c std::basic_filebuf::close().  If that function
       *  fails, @c failbit is set in the stream's error state.
      */
      void
      close()
      {
	if (!_M_filebuf.close())
	  this->setstate(std::ios_base::failbit);
      }
    };


  // [27.8.1.8] Template class basic_ofstream
  /**
   *  @brief  Controlling output for files.
   *
   *  This class supports reading from named files, using the inherited
   *  functions from std::basic_ostream.  To control the associated
   *  sequence, an instance of std::basic_filebuf is used, which this page
   *  refers to as @c sb.
  */
  template<typename _CharT, typename _Traits = std::char_traits<_CharT> >
    class basic_ofstream_utf8 : public std::basic_ostream<_CharT,_Traits>
    {
    public:
      // Types:
      typedef _CharT 					char_type;
      typedef _Traits 					traits_type;
      typedef typename traits_type::int_type 		int_type;
      typedef typename traits_type::pos_type 		pos_type;
      typedef typename traits_type::off_type 		off_type;

      // Non-standard types:
      typedef stdio_filebuf_utf8<char_type, traits_type> 	__filebuf_type;
      typedef std::basic_ostream<char_type, traits_type>	__ostream_type;

    private:
      __filebuf_type	_M_filebuf;

    public:
      // Constructors:
      /**
       *  @brief  Default constructor.
       *
       *  Initializes @c sb using its default constructor, and passes
       *  @c &sb to the base class initializer.  Does not open any files
       *  (you haven't given it a filename to open).
      */
      basic_ofstream_utf8(): __ostream_type(), _M_filebuf()
      { this->init(&_M_filebuf); }

      /**
       *  @brief  Create an output file stream.
       *  @param  s  Null terminated string specifying the filename.
       *  @param  mode  Open file in specified mode (see std::ios_base).
       *
       *  @c ios_base::out|ios_base::trunc is automatically included in
       *  @a mode.
       *
       *  Tip:  When using std::string to hold the filename, you must use
       *  .c_str() before passing it to this constructor.
      */
      explicit
      basic_ofstream_utf8(const char* __s,
		     std::ios_base::openmode __mode = std::ios_base::out|std::ios_base::trunc)
      : __ostream_type(), _M_filebuf()
      {
	this->init(&_M_filebuf);
	this->open(__s, __mode);
      }

      /**
       *  @brief  The destructor does nothing.
       *
       *  The file is closed by the filebuf object, not the formatting
       *  stream.
      */
      ~basic_ofstream_utf8()
      { }

      // Members:
      /**
       *  @brief  Accessing the underlying buffer.
       *  @return  The current basic_filebuf buffer.
       *
       *  This hides both signatures of std::basic_ios::rdbuf().
      */
      __filebuf_type*
      rdbuf() const
      { return const_cast<__filebuf_type*>(&_M_filebuf); }

      /**
       *  @brief  Wrapper to test for an open file.
       *  @return  @c rdbuf()->is_open()
      */
      bool
      is_open()
      { return _M_filebuf.is_open(); }

      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 365. Lack of const-qualification in clause 27
      bool
      is_open() const
      { return _M_filebuf.is_open(); }

      /**
       *  @brief  Opens an external file.
       *  @param  s  The name of the file.
       *  @param  mode  The open mode flags.
       *
       *  Calls @c std::basic_filebuf::open(s,mode|out|trunc).  If that
       *  function fails, @c failbit is set in the stream's error state.
       *
       *  Tip:  When using std::string to hold the filename, you must use
       *  .c_str() before passing it to this constructor.
      */
      void
      open(const char* __s,
	   std::ios_base::openmode __mode = std::ios_base::out | std::ios_base::trunc)
      {
	if (!_M_filebuf.open(convertUtf8(__s, __mode | std::ios_base::out), __mode | std::ios_base::out))
	  this->setstate(std::ios_base::failbit);
	else
	  // _GLIBCXX_RESOLVE_LIB_DEFECTS
	  // 409. Closing an fstream should clear error state
	  this->clear();
      }

      /**
       *  @brief  Close the file.
       *
       *  Calls @c std::basic_filebuf::close().  If that function
       *  fails, @c failbit is set in the stream's error state.
      */
      void
      close()
      {
        if (!_M_filebuf.close())
        this->setstate(std::ios_base::failbit);
      }
    };


  // [27.8.1.11] Template class basic_fstream
  /**
   *  @brief  Controlling input and output for files.
   *
   *  This class supports reading from and writing to named files, using
   *  the inherited functions from std::basic_iostream.  To control the
   *  associated sequence, an instance of std::basic_filebuf is used, which
   *  this page refers to as @c sb.
  */
  template<typename _CharT, typename _Traits = std::char_traits<_CharT> >
    class basic_fstream_utf8 : public std::basic_iostream<_CharT, _Traits>
    {
    public:
      // Types:
      typedef _CharT 					char_type;
      typedef _Traits 					traits_type;
      typedef typename traits_type::int_type 		int_type;
      typedef typename traits_type::pos_type 		pos_type;
      typedef typename traits_type::off_type 		off_type;

      // Non-standard types:
      typedef stdio_filebuf_utf8<char_type, traits_type> 	__filebuf_type;
      typedef std::basic_ios<char_type, traits_type>		__ios_type;
      typedef std::basic_iostream<char_type, traits_type>	__iostream_type;

    private:
      __filebuf_type	_M_filebuf;

    public:
      // Constructors/destructor:
      /**
       *  @brief  Default constructor.
       *
       *  Initializes @c sb using its default constructor, and passes
       *  @c &sb to the base class initializer.  Does not open any files
       *  (you haven't given it a filename to open).
      */
      basic_fstream_utf8()
      : __iostream_type(), _M_filebuf()
      { this->init(&_M_filebuf); }

      /**
       *  @brief  Create an input/output file stream.
       *  @param  s  Null terminated string specifying the filename.
       *  @param  mode  Open file in specified mode (see std::ios_base).
       *
       *  Tip:  When using std::string to hold the filename, you must use
       *  .c_str() before passing it to this constructor.
      */
      explicit
      basic_fstream_utf8(const char* __s,
		    std::ios_base::openmode __mode = std::ios_base::in | std::ios_base::out)
      : __iostream_type(NULL), _M_filebuf()
      {
	this->init(&_M_filebuf);
	this->open(__s, __mode);
      }

      /**
       *  @brief  The destructor does nothing.
       *
       *  The file is closed by the filebuf object, not the formatting
       *  stream.
      */
      ~basic_fstream_utf8()
      { }

      // Members:
      /**
       *  @brief  Accessing the underlying buffer.
       *  @return  The current basic_filebuf buffer.
       *
       *  This hides both signatures of std::basic_ios::rdbuf().
      */
      __filebuf_type*
      rdbuf() const
      { return const_cast<__filebuf_type*>(&_M_filebuf); }

      /**
       *  @brief  Wrapper to test for an open file.
       *  @return  @c rdbuf()->is_open()
      */
      bool
      is_open()
      { return _M_filebuf.is_open(); }

      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 365. Lack of const-qualification in clause 27
      bool
      is_open() const
      { return _M_filebuf.is_open(); }

      /**
       *  @brief  Opens an external file.
       *  @param  s  The name of the file.
       *  @param  mode  The open mode flags.
       *
       *  Calls @c std::basic_filebuf::open(s,mode).  If that
       *  function fails, @c failbit is set in the stream's error state.
       *
       *  Tip:  When using std::string to hold the filename, you must use
       *  .c_str() before passing it to this constructor.
      */
      void
      open(const char* __s,
	   std::ios_base::openmode __mode = std::ios_base::in | std::ios_base::out)
      {
	if (!_M_filebuf.open(convertUtf8(__s, __mode), __mode))
	  this->setstate(std::ios_base::failbit);
	else
	  // _GLIBCXX_RESOLVE_LIB_DEFECTS
	  // 409. Closing an fstream should clear error state
	  this->clear();
      }

      /**
       *  @brief  Close the file.
       *
       *  Calls @c std::basic_filebuf::close().  If that function
       *  fails, @c failbit is set in the stream's error state.
      */
      void
      close()
      {
	if (!_M_filebuf.close())
	  this->setstate(std::ios_base::failbit);
      }
    };


typedef basic_ifstream_utf8<char> ifstream_utf8;
typedef basic_ofstream_utf8<char> ofstream_utf8;
typedef basic_fstream_utf8<char> fstream_utf8;


#endif // #ifndef WIN32 / else

#endif

