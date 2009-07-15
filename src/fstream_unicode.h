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

Drop-in replacements for ifstream, ofstream, and fstream, which take Unicode
strings for filenames (instead of the char* strings in the system codepage
that MinGW uses.)

(There are ifstream, ofstream, and fstream correspondents, but only fstream
is mentioned, for brevity.)

Provides the class fstream_unicode, which allows opening of a file with the
name given as a UTF-8 or a UTF-16 string.

Provides fstream_utf8, which on Windows is a typedef for fstream_unicode, while
elsewhere it's a typedef for std::basic_fstream (currently the assumption is
that outside Windows everything is UTF-8, although the older a Linux system is,
the more likely it is to have non-UTF-8 file names.)

Normally fstream_utf8 seems the best to use, because Linux users are shielded
from bugs in fstream_unicode. However, the situation of a particular project
may dictate otherwise.

Currently using wchar_t* to open files on Linux throws an exception. It would
be easy to implement, but there's no use for it right now.

While the name reflects the intended use, fstream_unicode can do more than just
open files with names given as Unicode strings: you can pass an existing file
descriptor on its constructor, or you can add a specialization of unicodeOpenHlp
that turns some custom objects into file descriptors or into UTF-8 or UTF-16
strings, and then pass those objects to fstream_unicode's constructor or to
its open() function. For example you can easily make fstream_unicode accept a
std::string, without the need of calling c_str().

*/



#include  <ext/stdio_filebuf.h>
#include  <istream>
#include  <ostream>

#include  <fcntl.h> // for open()


template<class T>
int unicodeOpenHlp(T handler, std::ios_base::openmode __mode);




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


// needed because the base class doesn't have open()
template<typename _CharT, typename _Traits = std::char_traits<_CharT> >
class stdio_filebuf_open : public __gnu_cxx::stdio_filebuf<_CharT, _Traits>
{
public:
    stdio_filebuf_open() : __gnu_cxx::stdio_filebuf<_CharT, _Traits>() {}

    stdio_filebuf_open(int __fd, std::ios_base::openmode __mode, size_t __size = static_cast<size_t>(BUFSIZ)) : __gnu_cxx::stdio_filebuf<_CharT, _Traits>(__fd, __mode, __size) {}

    /*override*/ ~stdio_filebuf_open() {}
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

    //using __gnu_cxx::stdio_filebuf<_CharT, _Traits>::open;

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
    class basic_ifstream_unicode : public std::basic_istream<_CharT, _Traits>
    {
    public:
      // Types:
      typedef _CharT 					char_type;
      typedef _Traits 					traits_type;
      typedef typename traits_type::int_type 		int_type;
      typedef typename traits_type::pos_type 		pos_type;
      typedef typename traits_type::off_type 		off_type;

      // Non-standard types:
      typedef stdio_filebuf_open<char_type, traits_type> 	__filebuf_type;
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
      basic_ifstream_unicode() : __istream_type(), _M_filebuf()
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
      template<class T>
      explicit
      basic_ifstream_unicode(T x, std::ios_base::openmode __mode = std::ios_base::in)
      : __istream_type(), _M_filebuf()
      {
	this->init(&_M_filebuf);
	this->open(x, __mode);
      }

      /**
       *  @brief  The destructor does nothing.
       *
       *  The file is closed by the filebuf object, not the formatting
       *  stream.
      */
      ~basic_ifstream_unicode()
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
      template<class T>
      void
      open(T x, std::ios_base::openmode __mode = std::ios_base::in)
      {
	if (!_M_filebuf.open(unicodeOpenHlp(x, __mode | std::ios_base::in), __mode | std::ios_base::in))
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
    class basic_ofstream_unicode : public std::basic_ostream<_CharT,_Traits>
    {
    public:
      // Types:
      typedef _CharT 					char_type;
      typedef _Traits 					traits_type;
      typedef typename traits_type::int_type 		int_type;
      typedef typename traits_type::pos_type 		pos_type;
      typedef typename traits_type::off_type 		off_type;

      // Non-standard types:
      typedef stdio_filebuf_open<char_type, traits_type> 	__filebuf_type;
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
      basic_ofstream_unicode(): __ostream_type(), _M_filebuf()
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
      template<class T>
      explicit
      basic_ofstream_unicode(T x,
		     std::ios_base::openmode __mode = std::ios_base::out|std::ios_base::trunc)
      : __ostream_type(), _M_filebuf()
      {
	this->init(&_M_filebuf);
	this->open(x, __mode);
      }

      /**
       *  @brief  The destructor does nothing.
       *
       *  The file is closed by the filebuf object, not the formatting
       *  stream.
      */
      ~basic_ofstream_unicode()
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
      template<class T>
      void
      open(T x,
	   std::ios_base::openmode __mode = std::ios_base::out | std::ios_base::trunc)
      {
	if (!_M_filebuf.open(unicodeOpenHlp(x, __mode | std::ios_base::out), __mode | std::ios_base::out))
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
    class basic_fstream_unicode : public std::basic_iostream<_CharT, _Traits>
    {
    public:
      // Types:
      typedef _CharT 					char_type;
      typedef _Traits 					traits_type;
      typedef typename traits_type::int_type 		int_type;
      typedef typename traits_type::pos_type 		pos_type;
      typedef typename traits_type::off_type 		off_type;

      // Non-standard types:
      typedef stdio_filebuf_open<char_type, traits_type> 	__filebuf_type;
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
      basic_fstream_unicode()
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
      template<class T>
      explicit
      basic_fstream_unicode(T x,
		    std::ios_base::openmode __mode = std::ios_base::in | std::ios_base::out)
      : __iostream_type(NULL), _M_filebuf()
      {
	this->init(&_M_filebuf);
	this->open(x, __mode);
      }

      /**
       *  @brief  The destructor does nothing.
       *
       *  The file is closed by the filebuf object, not the formatting
       *  stream.
      */
      ~basic_fstream_unicode()
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
      template<class T>
      void
      open(T x,
	   std::ios_base::openmode __mode = std::ios_base::in | std::ios_base::out)
      {
	if (!_M_filebuf.open(unicodeOpenHlp(x, __mode), __mode))
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



#ifndef WIN32

    //inline const char* unicodeOpenHlp(const char* szUtf8Name) { return szUtf8Name; }
    //int unicodeOpenHlp(const char* szUtf8Name, std::ios_base::openmode __mode);
    #include <fstream>
    typedef std::basic_ifstream<char> ifstream_utf8;
    typedef std::basic_ofstream<char> ofstream_utf8;
    typedef std::basic_fstream<char> fstream_utf8;

#else

    typedef basic_ifstream_unicode<char> ifstream_utf8;
    typedef basic_ofstream_unicode<char> ofstream_utf8;
    typedef basic_fstream_unicode<char> fstream_utf8;

#endif // #ifndef WIN32 / else


typedef basic_ifstream_unicode<char> ifstream_unicode;
typedef basic_ofstream_unicode<char> ofstream_unicode;
typedef basic_fstream_unicode<char> fstream_unicode;


#endif

