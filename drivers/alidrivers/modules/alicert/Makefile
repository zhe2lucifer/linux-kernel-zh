#
# Makefile for the CERT(ASA and AKL) driver
#

obj-$(CONFIG_ALI_CERT) += alicert.o

alicert-y := ali_cert.o ali_cert_asa.o ali_cert_akl.o \
		ali_cert_utils.o ali_cert_asa_rpc.o ali_cert_akl_rpc.o \
		ali_cert_asa_splice.o ali_cert_asa_sbm.o \
		ali_cert_asa_page.o

alicert-$(CONFIG_DEBUG_FS) += ali_cert_akl_dbgfs.o ali_cert_asa_dbgfs.o		
