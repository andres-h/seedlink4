* template: $template
plugin 4chain$seedlink.4chain.id cmd="$seedlink.plugin_dir/chain4_plugin$seedlink._daemon_opt -v -f $seedlink.config_dir/4chain${seedlink.4chain.id}.xml"
             timeout = 600
             start_retry = 60
             shutdown_wait = 15

