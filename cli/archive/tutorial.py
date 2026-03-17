from argparse import ArgumentParser, Namespace

parser = ArgumentParser()

parser.add_argument('square', help='Squares a given number', type=int, default=0)


parser.add_argument('-v', '--verbose', help='Provides a verbose description', 
                    type=int, choices=[0, 1, 2])

args: Namespace = parser.parse_args()

''' 
parser.add_argument('-v', '--verbose', help='Provides a verbose description', 
                    action='store_true', required=False)
                    
if args.verbose:
    print(f'{args.square} squared is: {args.square**2}')
else:
    print((args.square)**2)
'''

if args.verbose == 0:
    print('Option 1')
elif args.verbose == 1:
    print('Option 2')
elif args.verbose == 2:
    print('Option 3')

print(args.square ** 2)