import os
import subprocess
import unittest


class TestCPrograms(unittest.TestCase):

    def setUp(self):
        # Ensure that 'make cc' is available in the path
        subprocess.run(['make', 'cc'], check=True)

    def generate_test(self, c_file, expected_output_file):
        # Define the test method
        def dynamic_test_method(self):
            executable_name = "a"

            # Compile the C program
            compile_command = f'./cc {c_file} --skip-semantics'
            subprocess.run(compile_command, shell=True, check=True)

            # Generate object file
            obj_command = f'llc -filetype=obj {executable_name}.ll -o {executable_name}.o'
            subprocess.run(obj_command, shell=True, check=True)

            # Link object file and create executable
            link_command = f'clang {executable_name}.o -o {executable_name}.out -pie'
            subprocess.run(link_command, shell=True, check=True)

            # Execute the program
            execution_command = f'./{executable_name}.out'
            process = subprocess.Popen(execution_command,
                                       stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE,
                                       shell=True)
            output, _ = process.communicate()
            return_code = process.returncode

            # Read the expected output
            with open(expected_output_file, 'r') as expected_file:
                expected_output = expected_file.read()

            # Check if the output matches the expected output and return code is 0
            self.assertEqual(return_code, 0,
                             f"Return code for {executable_name} is not 0")
            self.assertEqual(
                output.decode().strip(), expected_output.strip(),
                f"Output for {executable_name} does not match expected output")

        return dynamic_test_method


if __name__ == '__main__':
    # Get all .c files in the stress directory
    c_files = [
        filename for filename in os.listdir('./stress/')
        if filename.endswith('.c')
    ]

    c_files = sorted(c_files)
    print(c_files)

    # Create a TestSuite instance
    suite = unittest.TestSuite()

    # Create a TestCPrograms instance
    test_case = TestCPrograms()

    subprocess.run(['make', 'clean'], check=True)
    # Dynamically generate a test method for each .c file
    for c_file in c_files:
        expected_output_file = os.path.join('./stress/', c_file + '.expected')
        c_inp_file = os.path.join('./stress/', c_file)
        test_name = 'test_' + c_file[:-2]  # Remove the '.c' extension

        # Generate the test method
        test_method = test_case.generate_test(c_inp_file, expected_output_file)

        # Add the test method to the TestCPrograms class
        setattr(TestCPrograms, test_name, test_method)

        # Add the test method to the TestSuite
        suite.addTest(TestCPrograms(test_name))

    # Run the test suite
    unittest.TextTestRunner(failfast=True).run(suite)
