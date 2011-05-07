install: fpkbd-input 85-fpcskbd.rules
	install -m755 -s fpkbd-input /usr/local/bin
	install -m644 85-fpcskbd.rules /etc/udev/rules.d
