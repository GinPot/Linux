#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drmP.h>

#include "sunh5_drv.h"
#include "sunh5_framebuffer.h"



static int sunh5_de_atomic_check(struct drm_device *dev, struct drm_atomic_state *state)
{
	int ret;

	dev_dbg(dev->dev, "%s: Enter\n", __func__);

	ret = drm_atomic_helper_check_modeset(dev, state);
	if(ret)
		return ret;

	ret = drm_atomic_normalize_zpos(dev, state);
	if(ret)
		return ret;

	return drm_atomic_helper_check_planes(dev, state);
}

static int sunh5_de_drm_atomic_helper_commit(struct drm_device *dev,
			     struct drm_atomic_state *state,
			     bool nonblock)
{
	dev_dbg(dev->dev, "%s: Enter\n", __func__);
	return drm_atomic_helper_commit(dev,state,nonblock);
}

static struct drm_framebuffer *sunh5_de_drm_gem_fb_create(struct drm_device *dev, struct drm_file *file,
						   const struct drm_mode_fb_cmd2 *mode_cmd)
{
	dev_dbg(dev->dev, "%s: Enter\n", __func__);
	return drm_gem_fb_create(dev, file, mode_cmd);
}

static const struct drm_mode_config_funcs sunh5_de_mode_config_funcs = {
	.atomic_check	= sunh5_de_atomic_check,
	.atomic_commit	= sunh5_de_drm_atomic_helper_commit,
	.fb_create		= sunh5_de_drm_gem_fb_create,
};

static void sunh5_de_drm_atomic_helper_commit_tail_rpm(struct drm_atomic_state *old_state)
{
	dev_dbg(old_state->dev->dev, "%s: Enter\n", __func__);

	drm_atomic_helper_commit_tail_rpm(old_state);
}

static struct drm_mode_config_helper_funcs sunh5_de_mode_config_helpers = {
	.atomic_commit_tail	= sunh5_de_drm_atomic_helper_commit_tail_rpm,
};

void sunh5_framebuffer_init(struct drm_device *drm)
{
	dev_dbg(drm->dev, "%s: Enter\n", __func__);

	drm->mode_config.max_width	= 8192;
	drm->mode_config.max_height = 8192;

	drm->mode_config.funcs		= &sunh5_de_mode_config_funcs;
	drm->mode_config.helper_private = &sunh5_de_mode_config_helpers;

	drm_mode_config_reset(drm);
}
