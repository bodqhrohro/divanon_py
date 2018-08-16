#
#    divanon: the deanonymizer
#    Copyright (C) 2018  Bohdan "bodqhrohro" Horbeshko
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

import numpy as np
from random import random

def normalize_np_array(np_array):
    array_max = np.amax(np_array)
    if (array_max > 0):
        np_array = np_array / array_max
    return np_array

def randomly_split_list(in_list, first_part_len):
    first_part_len = int(first_part_len)
    list1 = []
    list2 = []
    first_part_probability = first_part_len / len(in_list)
    # firstly make a non-precise distribution
    for item in in_list:
        dice_roll = random()
        if dice_roll < first_part_probability:
            list1.append(item)
        else:
            list2.append(item)
    # now ensure the requested ratio
    return balance_lists(list1, list2, first_part_len)

def balance_lists(list1, list2, first_part_len):
    difference = first_part_len - len(list1)
    if difference > 0:
        list1 += list2[-difference:]
        list2 = list2[:-difference]
    else:
        list2 += list1[difference:]
        list1 = list1[:difference]
    return ( list1, list2 )
