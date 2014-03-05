/* -*-Mode: c++; -*- */
/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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
 * @file   tizgraphmgr.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph manager base class impl
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#include <tizosal.h>
#include <tizmacros.h>
#include <OMX_Component.h>

#include "tizgraphmgrcmd.h"
#include "tizgraph.h"
#include "tizgraphconfig.h"
#include "tizomxutil.h"
#include "tizgraphmgr.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graphmgr"
#endif

namespace graphmgr = tiz::graphmgr;

void *graphmgr::thread_func (void *p_arg)
{
  mgr *p_mgr = static_cast<mgr *>(p_arg);
  void *p_data = NULL;
  bool done = false;

  assert (NULL != p_mgr);

  (void)tiz_thread_setname (&(p_mgr->thread_), (char *)"tizgraphmgr");
  tiz_check_omx_err_ret_null (tiz_sem_post (&(p_mgr->sem_)));

  while (!done)
  {
    tiz_check_omx_err_ret_null (tiz_queue_receive (p_mgr->p_queue_, &p_data));

    assert (NULL != p_data);

    cmd *p_cmd = static_cast<cmd *>(p_data);
    done = mgr::dispatch_cmd (p_mgr, p_cmd);

    delete p_cmd;
  }

  tiz_check_omx_err_ret_null (tiz_sem_post (&(p_mgr->sem_)));
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Graph manager thread exiting...");

  return NULL;
}

//
// mgr
//
graphmgr::mgr::mgr (const uri_lst_t &file_list,
                    const error_callback_t &error_cback)
  : thread_ (),
    mutex_ (),
    sem_ (),
    p_queue_ (NULL),
    ops_ (this, file_list, error_cback),
    fsm_ (boost::msm::back::states_ << graphmgr::fsm::starting (&ops_)
                                    << graphmgr::fsm::restarting (&ops_)
                                    << graphmgr::fsm::stopping (&ops_),
          &ops_)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");
}

graphmgr::mgr::~mgr ()
{
}

OMX_ERRORTYPE
graphmgr::mgr::init ()
{
  tiz_mutex_init (&mutex_);
  tiz_sem_init (&sem_, 0);
  tiz_queue_init (&p_queue_, 10);
  tiz::omxutil::init ();

  fsm_.start ();

  /* Create the manager's thread */
  tiz_check_omx_err_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_err_ret_oom (
      tiz_thread_create (&thread_, 0, 0, thread_func, this));
  tiz_check_omx_err_ret_oom (tiz_mutex_unlock (&mutex_));

  // Let's wait until the manager's thread is ready to receive requests
  tiz_check_omx_err_ret_oom (tiz_sem_wait (&sem_));

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
graphmgr::mgr::deinit ()
{
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "Waiting until stopped...");
  tiz_check_omx_err_ret_oom (tiz_sem_wait (&sem_));
  void *p_result = NULL;
  tiz_thread_join (&thread_, &p_result);
  tiz::omxutil::deinit ();
  tiz_mutex_destroy (&mutex_);
  tiz_sem_destroy (&sem_);
  tiz_queue_destroy (p_queue_);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
graphmgr::mgr::start ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::start_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::next ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::next_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::prev ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::prev_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::fwd ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::fwd_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::rwd ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::rwd_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::volume (const int step)
{
  if (step == 0)
  {
    return OMX_ErrorNone;
  }

  if (step > 0)
  {
    return post_cmd (new graphmgr::cmd (graphmgr::vol_up_evt ()));
  }
  else
  {
    return post_cmd (new graphmgr::cmd (graphmgr::vol_down_evt ()));
  }
}

OMX_ERRORTYPE
graphmgr::mgr::mute ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::mute_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::pause ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::pause_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::stop ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::stop_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::graph_loaded ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::graph_loaded_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::graph_execd ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::graph_execd_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::graph_unloaded ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::graph_unlded_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::graph_end_of_play ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::graph_eop_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::graph_error (const OMX_ERRORTYPE error, const std::string &msg)
{
  bool is_internal_error = false;
  return post_cmd (
      new graphmgr::cmd (graphmgr::err_evt (error, msg, is_internal_error)));
}

OMX_ERRORTYPE
graphmgr::mgr::post_cmd (graphmgr::cmd *p_cmd)
{
  assert (NULL != p_cmd);
  assert (NULL != p_queue_);

  tiz_check_omx_err_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_err_ret_oom (tiz_queue_send (p_queue_, p_cmd));
  tiz_check_omx_err_ret_oom (tiz_mutex_unlock (&mutex_));

  return OMX_ErrorNone;
}

bool graphmgr::mgr::dispatch_cmd (graphmgr::mgr *p_mgr,
                                  const graphmgr::cmd *p_cmd)
{
  assert (NULL != p_mgr);
  assert (NULL != p_cmd);

  p_cmd->inject (p_mgr->fsm_);

  // Check for internal errors produced during the processing of the last
  // event. If any, inject an "internal" error event. This is fatal and shall
  // produce the termination of the state machine.
  if (OMX_ErrorNone != p_mgr->ops_.get_internal_error ())
  {
    TIZ_LOG (TIZ_PRIORITY_ERROR,
             "MGR error detected. Injecting err_evt (this is fatal)");
    bool is_internal_error = true;
    p_mgr->fsm_.process_event (graphmgr::err_evt (
        p_mgr->ops_.get_internal_error (),
        p_mgr->ops_.get_internal_error_msg (), is_internal_error));
  }
  if (p_mgr->fsm_.terminated_)
  {
    TIZ_LOG (TIZ_PRIORITY_NOTICE, "MGR fsm terminated");
    p_mgr->ops_.deinit ();
  }
  return p_mgr->fsm_.terminated_;
}
