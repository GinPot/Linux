#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <linux/kfifo.h>
#include <linux/of_device.h>
#include <linux/of_graph.h>
#include <linux/of_reserved_mem.h>

#include <drm/drmP.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_of.h>
#include <drm/drm_probe_helper.h>

#include "sunh5_drv.h"
#include "sunh5_framebuffer.h"



struct endpoint_list {
	DECLARE_KFIFO(fifo, struct device_node *, 16);
};




DEFINE_DRM_GEM_CMA_FOPS(sunh5_drv_fops);

static int sunh5_drm_gem_dumb_create(struct drm_file *file_priv, 
									struct drm_device *drm,
									struct drm_mode_create_dumb *args)
{
	/* The hardware only allows even pitches for YUV buffers. */
	args->pitch = ALIGN(DIV_ROUND_UP(args->width * args->bpp, 8), 2);

	dev_dbg(drm->dev, "%s: args->pitch=%d\n", __func__, args->pitch);

	return drm_gem_cma_dumb_create_internal(file_priv, drm, args);
}

static void sunh5_drm_gem_cma_free_object(struct drm_gem_object *gem_obj)
{
	dev_dbg(gem_obj->dev->dev, "%s: Enter\n", __func__);
	drm_gem_cma_free_object(gem_obj);
}

static int sunh5_drm_gem_prime_handle_to_fd(struct drm_device *dev,
			       struct drm_file *file_priv, uint32_t handle,
			       uint32_t flags,
			       int *prime_fd)
{
	dev_dbg(dev->dev, "%s: Enter\n", __func__);

	return drm_gem_prime_handle_to_fd(dev,file_priv,handle,flags,prime_fd);
}

static int sunh5_drm_gem_prime_fd_to_handle(struct drm_device *dev,
				  struct drm_file *file_priv, int prime_fd,
				  uint32_t *handle)
{
   dev_dbg(dev->dev, "%s: Enter\n", __func__);

   return drm_gem_prime_fd_to_handle(dev,file_priv,prime_fd,handle);
}

static struct drm_gem_object *sunh5_drm_gem_prime_import(struct drm_device *dev,
						  struct dma_buf *dma_buf)
{
	  dev_dbg(dev->dev, "%s: Enter\n", __func__);
	  return drm_gem_prime_import(dev,dma_buf);
}

static struct dma_buf *sunh5_drm_gem_prime_export(struct drm_device *dev,
					   struct drm_gem_object *obj,
					   int flags)
{
	  dev_dbg(dev->dev, "%s: Enter\n", __func__);
	  return drm_gem_prime_export(dev,obj,flags);
}

static struct sg_table *sunh5_drm_gem_cma_prime_get_sg_table(struct drm_gem_object *obj)
{
   dev_dbg(obj->dev->dev, "%s: Enter\n", __func__);
   return drm_gem_cma_prime_get_sg_table(obj);
}

static struct drm_gem_object *sunh5_drm_gem_cma_prime_import_sg_table(struct drm_device *dev,
				 struct dma_buf_attachment *attach,
				 struct sg_table *sgt)
{
   dev_dbg(dev->dev, "%s: Enter\n", __func__);
   return drm_gem_cma_prime_import_sg_table(dev,attach,sgt);
}

static void *sunh5_drm_gem_cma_prime_vmap(struct drm_gem_object *obj)
{
   dev_dbg(obj->dev->dev, "%s: Enter\n", __func__);
   return drm_gem_cma_prime_vmap(obj);
}

static void sunh5_drm_gem_cma_prime_vunmap(struct drm_gem_object *obj, void *vaddr)
{
   dev_dbg(obj->dev->dev, "%s: Enter\n", __func__);
   drm_gem_cma_prime_vunmap(obj,vaddr);
}

static int sunh5_drm_gem_cma_prime_mmap(struct drm_gem_object *obj,
			  struct vm_area_struct *vma)
{
   dev_dbg(obj->dev->dev, "%s: Enter\n", __func__);
   return drm_gem_cma_prime_mmap(obj,vma);
}


static struct drm_driver sunh5_drv_driver = {
	.driver_features = DRIVER_GEM | DRIVER_MODESET | DRIVER_PRIME | DRIVER_ATOMIC,

	/* Generic Operations */
	.fops						= &sunh5_drv_fops,
	.name						= "sun4i-drm",
	.desc						= "Allwinner sun4i Display Engine",
	.date						= "20200502",
	.major						= 1,
	.minor						= 0,

	/* GEM Operations */
	.dumb_create				= sunh5_drm_gem_dumb_create,
	.gem_free_object_unlocked	= sunh5_drm_gem_cma_free_object,
	.gem_vm_ops					= &drm_gem_cma_vm_ops,

	/* PRIME Operations */
	.prime_handle_to_fd			= sunh5_drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle			= sunh5_drm_gem_prime_fd_to_handle,
	.gem_prime_import			= sunh5_drm_gem_prime_import,
	.gem_prime_export			= sunh5_drm_gem_prime_export,
	.gem_prime_get_sg_table		= sunh5_drm_gem_cma_prime_get_sg_table,
	.gem_prime_import_sg_table	= sunh5_drm_gem_cma_prime_import_sg_table,
	.gem_prime_vmap				= sunh5_drm_gem_cma_prime_vmap,
	.gem_prime_vunmap			= sunh5_drm_gem_cma_prime_vunmap,
	.gem_prime_mmap				= sunh5_drm_gem_cma_prime_mmap,
};

static int sunh5_drv_bind(struct device *dev)
{
	struct drm_device *drm;
	struct sunh5_drv *drv;
	int ret;

	dev_dbg(dev, "%s: Enter\n", __func__);

	drm = drm_dev_alloc(&sunh5_drv_driver, dev);
	if(IS_ERR(drm))
		return PTR_ERR(drm);

	drv = devm_kzalloc(dev, sizeof(*drv), GFP_KERNEL);
	if(!drv){
		ret = -ENOMEM;
		goto free_drm;
	}

	dev_set_drvdata(dev, drm);
	drm->dev_private = drv;
	INIT_LIST_HEAD(&drv->frontend_list);
	INIT_LIST_HEAD(&drv->engine_list);
	INIT_LIST_HEAD(&drv->tcon_list);

	ret = of_reserved_mem_device_init(dev);
	if(ret && ret != -ENODEV){
		dev_err(drm->dev, "%s: Couldn't claim our memory region\n", __func__);
		goto free_drm;
	}
		
	drm_mode_config_init(drm);
	drm->mode_config.allow_fb_modifiers = true;

	ret = component_bind_all(drm->dev, drm);
	if(ret){
		dev_err(drm->dev, "%s: Couldn't bind all pipelines components\n", __func__);
		goto cleanup_mode_config;
	}

	/* drm_vbank_init calls kcalloc, which can fail */
	ret = drm_vblank_init(drm, drm->mode_config.num_crtc);
	if(ret)
		goto cleanup_mode_config;

	drm->irq_enabled = true;

	/* Remove early framebuffers (ie. simplefb) */
	drm_fb_helper_remove_conflicting_framebuffers(NULL, "sun4i-drm-fb", false);

	sunh5_framebuffer_init(drm);

	/* Enable connectors polling */
	drm_kms_helper_poll_init(drm);

	ret = drm_dev_register(drm, 0);
	if(ret)
		goto finish_poll;

	drm_fbdev_generic_setup(drm, 32);

	return 0;

finish_poll:
	drm_kms_helper_poll_fini(drm);
cleanup_mode_config:
	drm_mode_config_cleanup(drm);
	of_reserved_mem_device_release(dev);
free_drm:
	drm_dev_put(drm);

	return ret;
}

static void sunh5_drv_unbind(struct device *dev)
{
	struct drm_device *drm = dev_get_drvdata(dev);

	drm_dev_unregister(drm);
	drm_kms_helper_poll_fini(drm);
	drm_atomic_helper_shutdown(drm);
	drm_mode_config_cleanup(drm);

	component_unbind_all(dev, NULL);

	of_reserved_mem_device_release(dev);

	drm_dev_put(drm);
}

static const struct component_master_ops sunh5_drv_master_ops = {
	.bind	= sunh5_drv_bind,
	.unbind	= sunh5_drv_unbind,
};

static bool sunh5_drv_node_is_connector(struct device_node *node)
{
	return of_device_is_compatible(node, "hdmi-connector");
}

static int compare_of(struct device *dev, void *data)
{
	dev_dbg(dev, "%s: Comparing of node %s witch %s\n", __func__, dev->of_node->name, ((struct device_node*)data)->name);

	return dev->of_node == data;
}

static void sunh5_drv_traverse_endpoints(struct device *dev, struct endpoint_list *list,
												struct device_node *node,
												int prot_id)
{
	struct device_node *ep, *remote, *prot;

	prot = of_graph_get_port_by_id(node, prot_id);
	if(!prot){
		dev_err(dev, "%s: No output to bind on prot %d\n", __func__, prot_id);
		return;
	}

	dev_dbg(dev, "%s: get prot %s\n", __func__, prot->name);

	for_each_available_child_of_node(prot, ep) {
		remote = of_graph_get_remote_port_parent(ep);
		if(!remote){
			dev_err(dev, "%s: Error retrieving the output node\n", __func__);
			continue;
		}

		dev_dbg(dev, "%s: get remote port parent %s\n", __func__, remote->name);

		kfifo_put(&list->fifo, remote);
	}	
}

static int sunh5_drv_add_endpoint(struct device *dev,
										struct endpoint_list *list,
										struct component_match **match,
										struct device_node *node)
{
	int count = 0;

	/*
	 * The connectors will be the last nodes in our pipeline, we
	 * can just bail out.
	 */
	if(sunh5_drv_node_is_connector(node))
		return 0;

	if(of_device_is_available(node)){
		/* Add current component */
		dev_dbg(dev, "%s: Adding component %s\n", __func__, node->name);
		drm_of_component_match_add(dev, match, compare_of, node);
		count++;
	}

	/* each node has at least one output */
	sunh5_drv_traverse_endpoints(dev, list, node, 1);

	return count;
}

static int sunh5_drv_probe(struct platform_device *pdev)
{
	struct component_match *match = NULL;
	struct device_node *np = pdev->dev.of_node, *endpoint;
	struct endpoint_list list;
	int i, ret, count = 0;

	dev_dbg(&pdev->dev, "%s: Enter\n", __func__);

	INIT_KFIFO(list.fifo);

	for(i = 0; ; i++){
		struct device_node *pipeline = of_parse_phandle(np, "allwinner,pipelines", i);

		if(!pipeline)
			break;

		dev_dbg(&pdev->dev, "%s: pipeline=%s\n", __func__, pipeline->name);

		kfifo_put(&list.fifo, pipeline);
	}

	while(kfifo_get(&list.fifo, &endpoint)){
		/* process this endpoint */
		ret = sunh5_drv_add_endpoint(&pdev->dev, &list, &match, endpoint);

		/* sunh5_drv_add_endpoint can fail to allocate memory */
		if(ret < 0)
			return ret;

		count += ret;
	}

	if(count)
		return component_master_add_with_match(&pdev->dev, &sunh5_drv_master_ops, match);
	else       
		return 0;
	
}

static int sunh5_drv_remove(struct platform_device *pdev)
{
	dev_dbg(&pdev->dev, "%s: Enter\n", __func__);

	component_master_del(&pdev->dev, &sunh5_drv_master_ops);

	return 0;
}

static const struct of_device_id sunh5_drv_of_table[] = {
	{ .compatible = "allwinner,sunh5-display-engine" },
	{  }
};
MODULE_DEVICE_TABLE(of, sunh5_drv_of_table);

static struct platform_driver sunh5_drv_platform_driver = {
	.probe				= sunh5_drv_probe,
	.remove				= sunh5_drv_remove,
	.driver				= {
		.name			= "sunh5-drm",
		.of_match_table	= sunh5_drv_of_table,
	},
};
module_platform_driver(sunh5_drv_platform_driver);

MODULE_DESCRIPTION("Allwinner H5 Display Engine DRM/KMS Driver");
MODULE_AUTHOR("GinPot <GinPot@xx.com>");
MODULE_LICENSE("GLP");
