Jampfiles should replace these BeIDE projects files in the future.
But, beware:
- the net_server binary should symlinked as _APP_ in each sub-drectories, so that
  each module can find the kernel module API exported in this app.
- the net_server will try to load the modules from a local add-ons folder, which should
  be build by hand today: symlink each module like this:

  net_server
  add-ons\
    generic\
      memory_pool -> ../../memory_pool/memory_pool
    network\
      stack -> ../../stack/stack
      protocols\
      interfaces\
        ethernet -> ../../../ethernet/ethernet
        loopback -> ../../../loopback/loopback
  
