##
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 # http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
 ##

'''
Run test:
>> pytest -q python/test/test_csv_read_options.py
'''

def test_check_csv_read_opts():
    from pycylon.io.csv_read_config import CSVReadOptions
    csv_read_options = CSVReadOptions().use_threads(True).block_size(1 << 30)


