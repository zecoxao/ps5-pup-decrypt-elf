# ps5-pup-decrypt
ps5 pup decrypt by asking nicely ps5 to do it. 

## Credits

* SpecterDev: for sdk files
* LeGend: for assistance on testing
* Scene-Collective/l0lhax/idc: for original code
* Anonymous: For help in getting the required coolstuff to reverse the protocol
* LemonHaze: For RE help

## Usage

place your ps5 update file in /mnt/usb0/safe.PS5UPDATE.PUP (this is root of your pendrive, file safe.PS5UPDATE.PUP)

plug the pendrive to the ps5.

let it do its thing (warning: this requires elevated privileges for most packages!)

either one or two files will be produced (PS5UPDATE1.PUP.dec and PS5UPDATE2.PUP.dec) depending on what file you have (sys or recovery)

## Requirements

- Ubuntu 20.04 with ``gcc (or WSL alternative)``

- make

## Actions

https://github.com/zecoxao/ps5-pup-decrypt/actions/

## Not Decrypted Files (system, system_ex, wlan)

it will only work on pups that are on:

- same version: e.g (5.00 on 5.00 pup)

- bigger versions but of the same .XX: e.g (5.02 on 5.50 pup, but NOT 6.00 pup)

for ssd0 system ssd0 system_ex AND wlan
