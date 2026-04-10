SeedLink4 implements the `FDSN SeedLink 4.0 <https://docs.fdsn.org/projects/seedlink/>`_ protocol and supports the `miniSEED 3 <https://docs.fdsn.org/projects/miniseed3/>`_ data format. SeedLink 3.x and miniSEED 2.x are also supported. The following applications are included:

seedlink4
    The SeedLink4 server
sl3plug
    An application that runs SeedLink plugins and feeds data to the server. All SeedLink 3.x plugins are supported.
chain4_plugin
    A version of chain_plugin that supports SeedLink4 and miniSEED 3.
slarchive4
    A version of slarchive that supports SeedLink4 and miniSEED 3.

Configuration
-------------
SeedLink4 does not have own bindings, but relies on "seedlink" bindings and plugin configuration created by the "seedlink" module. It does have own configuration file, seedlink4.cfg, which follows SeisComP conventions. If the configuration file does not exist, default settings are used.

SeedLink4 needs database connection for bindings and inventory (station desctiptions). New stations are added as soon as data is received, even if bindings and inventory does not exist.

If a working SeedLink 3.x configuration exists, a switchover to SeedLink4 can be done as follows:

* seiscomp stop seedlink
* seiscomp start seedlink4
* seiscomp start sl3plug

To support miniSEED 3, replace chain_plugin by chain4_plugin and slarchive by slarchive4. This can be done on a station-by-station basis (using bindings).
