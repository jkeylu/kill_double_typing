# kill_double_typing

## How to compile

```sh
g++ -std=c++11 -O2 -Wall -o kill_double_typing kill_double_typing.cpp -framework ApplicationServices
```

## How to use

```sh
./kill_double_typing --delay-all-keys --default-delay-duration 40 --delay-key n:65 --delay-key j:60 --delay-key x:50
```

## How to run as service

```sh
cp kill_double_typing /usr/local/bin/
sudo chown root:wheel /usr/local/bin/kill_double_typing
sudo cp kill_double_typing.plist /Library/LaunchAgents/
sudo launchctl bootstrap system /Library/LaunchAgents/kill_double_typing.plist

# stop the service
# sudo launchctl bootout system /Library/LaunchAgents/kill_double_typing.plist
```

## Troubleshooting

Everytime you get following error:

```
failed to create event tap
```

1. Go to "System settings" > "Privacy & Security" > "Accessibility".
2. Remove "kill_double_typing" (if exists), then add "kill_double_typing".

## Acknowledgement

The initial code was copied from "[解决 MacBook 键盘双击问题](https://sf-zhou.github.io/productivity/solve_macbook_typing_double.html)".
