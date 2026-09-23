#!/usr/bin/env python
import jinja2
import json
import sys
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("source", type=str)
parser.add_argument("context", type=str)
parser.add_argument("target", type=str)

args = parser.parse_args()


if __name__ == '__main__':

    j2loader = jinja2.FileSystemLoader("./")
    j2environment = jinja2.Environment(loader=j2loader,
                                       trim_blocks=True,
                                       lstrip_blocks=True,
                                       undefined=jinja2.StrictUndefined)
    try:
        j2template = j2environment.get_or_select_template(args.source)

        Context = json.loads(json.loads(args.context))

        rendered = j2template.render(**Context)
        with open(args.target, 'w') as target:
            target.write(rendered)

    except Exception as e:
        print(f"Erred with configuration:", file=sys.stderr)
        print(f"  template_path: {args.source}", file=sys.stderr)
        print(f"  context_json: {args.context}", file=sys.stderr)
        import os
        print(f"  cwd: {os.getcwd()}", file=sys.stderr)
        print(f"  ls: {os.listdir('.')}", file=sys.stderr)
        raise e
