/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
 *
 * This file is part of Tizonia
 *
 * Tizonia is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Tizonia is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Tizonia.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   tizyoutube.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple YouTube audio client library
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <boost/lexical_cast.hpp>

#include "tizyoutube.hpp"

namespace bp = boost::python;

/* This macro assumes the existence of an "int rc" local variable */
#define try_catch_wrapper(expr)                                  \
  do                                                             \
    {                                                            \
      try                                                        \
        {                                                        \
          (expr);                                                \
        }                                                        \
      catch (bp::error_already_set & e)                          \
        {                                                        \
          PyErr_PrintEx (0);                                     \
          rc = 1;                                                \
        }                                                        \
      catch (const std::exception &e)                            \
        {                                                        \
          std::cerr << e.what ();                                \
        }                                                        \
      catch (...)                                                \
        {                                                        \
          std::cerr << std::string ("Unknown exception caught"); \
        }                                                        \
    }                                                            \
  while (0)

namespace
{
  void init_youtube (boost::python::object &py_main,
                    boost::python::object &py_global)
  {
    Py_Initialize ();

    // Import the YouTube proxy module
    py_main = bp::import ("tizyoutubeproxy");

    // Retrieve the main module's namespace
    py_global = py_main.attr ("__dict__");
  }

  void start_youtube (boost::python::object &py_global,
                     boost::python::object &py_youtube_proxy,
                     const std::string &api_key)
  {
    bp::object pyyoutubeproxy = py_global["tizyoutubeproxy"];
    py_youtube_proxy
        = pyyoutubeproxy (api_key.c_str ());
  }
}

tizyoutube::tizyoutube (const std::string &api_key)
  : api_key_ (api_key)
{
}

tizyoutube::~tizyoutube ()
{
}

int tizyoutube::init ()
{
  int rc = 0;
  try_catch_wrapper (init_youtube (py_main_, py_global_));
  return rc;
}

int tizyoutube::start ()
{
  int rc = 0;
  try_catch_wrapper (
      start_youtube (py_global_, py_youtube_proxy_, api_key_));
  return rc;
}

void tizyoutube::stop ()
{
  int rc = 0;
  // try_catch_wrapper (py_youtube_proxy_.attr ("logout")());
  (void)rc;
}

void tizyoutube::deinit ()
{
  // boost::python doesn't support Py_Finalize() yet!
}

int tizyoutube::play_audio_stream (const std::string &url_or_id)
{
  int rc = 0;
  try_catch_wrapper (
      py_youtube_proxy_.attr ("enqueue_audio_stream")(bp::object (url_or_id)));
  return rc;
}

int tizyoutube::play_audio_playlist (const std::string &url_or_id)
{
  int rc = 0;
  try_catch_wrapper (
      py_youtube_proxy_.attr ("enqueue_audio_playlist")(bp::object (url_or_id)));
  return rc;
}

const char *tizyoutube::get_next_url (const bool a_remove_current_url)
{
  current_url_.clear ();
  try
    {
      if (a_remove_current_url)
        {
          py_youtube_proxy_.attr ("remove_current_url")();
        }
      const char *p_next_url
          = bp::extract< char const * >(py_youtube_proxy_.attr ("next_url")());
      current_url_.assign (p_next_url);
      if (!p_next_url || get_current_stream ())
        {
          current_url_.clear ();
        }
    }
  catch (bp::error_already_set &e)
    {
      PyErr_PrintEx (0);
    }
  catch (...)
    {
    }
  return current_url_.empty () ? NULL : current_url_.c_str ();
}

const char *tizyoutube::get_prev_url (const bool a_remove_current_url)
{
  current_url_.clear ();
  try
    {
      if (a_remove_current_url)
        {
          py_youtube_proxy_.attr ("remove_current_url")();
        }
      const char *p_prev_url
          = bp::extract< char const * >(py_youtube_proxy_.attr ("prev_url")());
      current_url_.assign (p_prev_url);
      if (!p_prev_url || get_current_stream ())
        {
          current_url_.clear ();
        }
    }
  catch (bp::error_already_set &e)
    {
      PyErr_PrintEx (0);
    }
  catch (...)
    {
    }
  return current_url_.empty () ? NULL : current_url_.c_str ();
}

void tizyoutube::clear_queue ()
{
  int rc = 0;
  try_catch_wrapper (py_youtube_proxy_.attr ("clear_queue")());
  (void)rc;
}

void tizyoutube::set_playback_mode (const playback_mode mode)
{
  int rc = 0;
  switch(mode)
    {
    case PlaybackModeNormal:
      {
        try_catch_wrapper (py_youtube_proxy_.attr ("set_play_mode")("NORMAL"));
      }
      break;
    case PlaybackModeShuffle:
      {
        try_catch_wrapper (py_youtube_proxy_.attr ("set_play_mode")("SHUFFLE"));
      }
      break;
    default:
      {
        assert (0);
      }
      break;
    };
  (void)rc;
}

int tizyoutube::get_current_stream ()
{
  int rc = 0;
//   current_stream_title_.clear ();

//   const bp::tuple &info1 = bp::extract< bp::tuple >(
//       py_youtube_proxy_.attr ("current_stream_title")());
//   const char *p_name = bp::extract< char const * >(info1[0]);

//   if (p_name)
//     {
//       current_stream_title_.assign (p_name);
//     }

//   if (p_name)
//      {
//         rc = 0;
//      }

  return rc;
}
