(add-hook
  'nxml-mode-hook
  (lambda ()
    (setq rng-schema-locating-files-default
          (append '("/docbook-sandbox/xsl/locatingrules.xml")
                  rng-schema-locating-files-default ))))
