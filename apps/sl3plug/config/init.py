import os
import seiscomp.kernel

class Module(seiscomp.kernel.Module):
    def __init__(self, env):
        seiscomp.kernel.Module.__init__(self, env, env.moduleName(__file__))

        self.pkgroot = self.env.SEISCOMP_ROOT
        self.config_dir = os.path.join(self.pkgroot, "var", "lib", "seedlink")

        # Set kill timeout to 5 minutes
        self.killTimeout = 300

        # Start after SL4
        self.order = 200

    def _run(self):
        if self.env.syslog:
            daemon_opt = '-D '
        else:
            daemon_opt = ''

        daemon_opt += "-v -f " + os.path.join(self.config_dir, "seedlink.ini")

        return self.env.start(self.name, self.env.binaryFile(self.name),
                              daemon_opt,
                              not self.env.syslog)

    def isRunning(self):
        return self.env.tryLock("seedlink") == False
        
    def stop(self):
        if not self.isRunning():
            self.env.log("%s is not running" % self.name)
            return 1

        self.env.log("shutting down %s" % self.name)
        # Default timeout to 10 seconds
        return self.env.stop("seedlink", self.killTimeout)

    def supportsAliases(self):
        return False

    def requiresKernelModules(self):
        return False

    def updateConfig(self):
        return 0

    def printCrontab(self):
        return 0

