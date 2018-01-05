import numpy as np
import tempfile, subprocess, shlex, os, re

class CMUWrapper:
    def _writeLine(self, line: str):
        self.hFile.write(line + '\n')

    def _writeDataLine(self, xs: np.array, y: np.array):
        self._writeLine(', '.join(map(lambda n: '{:.10f}'.format(n), xs)) + ' => ' + str(y) + ';')

    def _writeKeyVal(self, key: str, val):
        self.hFile.write(key + ': ' + str(val) + ';\n')

    def generateTrainSet(self, filename: str, X: np.array, y: np.array, num_ys: int):
        if not len(X) or not len(y):
            return

        try:
            self.hFile = open(filename, 'w')

            self._writeLine('$SETUP')
            self._writeKeyVal('PROTOCOL', 'IO')
            self._writeKeyVal('OFFSET', 0)
            self._writeKeyVal('INPUTS', len(X[0]))
            self._writeKeyVal('OUTPUTS', 1)
            for i, _ in enumerate(X[0]):
                self._writeKeyVal('IN [' + str(i+1) + ']', 'CONT {0..1}')
            self._writeKeyVal('OUT [1]', 'ENUM {' + ",".join([str(i) for i in range(num_ys)]) + '}')

            self._writeLine('$TRAIN')

            for i, X_r in enumerate(X):
                self._writeDataLine(X_r, y[i])

            self.hFile.close()
        except:
            print("Can't write dataset!")

        self.hFile = None

    def estimateByTrainFile(self, filename: str, X: np.array):
        self.hFile = tempfile.NamedTemporaryFile(mode='a', suffix='.data')
        stableFile = open(filename, 'r')
        self.hFile.write(stableFile.read())

        self._writeLine('$TEST')
        for i, X_r in enumerate(X):
            self._writeDataLine(X_r, 0)

        FNULL = open(os.devnull, 'w')
        subprocess.run(['./vendor/cascor/v110/cascor', shlex.quote(self.hFile.name), './vendor/cascor/v110/cascor.cfg'], stdout=FNULL, stderr=subprocess.STDOUT)

        self.hFile.close()

        # now read the cascor's hardcoded output file
        self.hFile = open('test.results', 'r')
        result = np.array([np.array(re.findall(r'\((-?[\d]+\.[\d]+)\)', line)).argmax() for line in self.hFile.readlines()])
        self.hFile.close()
        self.hFile = None

        return result
