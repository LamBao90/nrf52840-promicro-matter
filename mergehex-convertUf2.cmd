python -c "from intelhex import IntelHex; ih = IntelHex('build/mcuboot/zephyr/zephyr.hex'); ih.loadhex('build/matter_factory_data/zephyr/factory_data.hex'); ih.loadhex('build/tutorial/zephyr/zephyr.signed.hex'); ih.write_hex_file('uf2.hex')" 
pause 
python C:/ncs/v3.4.1/zephyr/scripts/build/uf2conv.py uf2.hex --family 0xADA52840 --convert --output uf2.uf2
pause