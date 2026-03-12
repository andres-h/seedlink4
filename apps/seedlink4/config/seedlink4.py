import os
import seiscomp.kernel


class Module(seiscomp.kernel.Module):
    def __init__(self, env):
        seiscomp.kernel.Module.__init__(self, env, env.moduleName(__file__))

    def _run(self):
        try: os.makedirs(os.path.join(self.env.SEISCOMP_ROOT, "var", "lib", self.name))
        except: pass

        try: os.makedirs(os.path.join(self.env.SEISCOMP_ROOT, "var", "run", self.name))
        except: pass

        return self.env.start(self.name, self.env.binaryFile(self.name), self._get_start_params())

    def updateConfigProxy(self):
        return "trunk"

    def updateConfig(self):
        # By default the "trunk" module must be configured to write the
        # bindings into the database
        return 0

    def supportsAliases(self):
        # The default handler does not support aliases
        return True
