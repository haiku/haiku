Jampfiles should replace these BeIDE projects files in the future.
But, beware:
- the net_stack_server binary should be symlinked as _APP_ in each sub-drectories, so 
  each module can be linked against it to find the kernel module API exported in this app.
- the net_stack_server will try to load the modules from a local add-ons folder, which should
  be setup by hands today: please symlink each module like this:

  net_stack_server
  add-ons\
    generic\
      memory_pool -> ../../memory_pool/memory_pool
      atomizer -> ../../atomizer/atomizer
    network\
      stack -> ../../stack/stack
      layers\
        protocols\
          arp -> ../../../../arp/arp
        interfaces\
          ethernet -> ../../../../ethernet/ethernet
          loopback -> ../../../../loopback/loopback
  
