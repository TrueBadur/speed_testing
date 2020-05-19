NAME = test_speed

FILES=main.cpp
OPTIMISATION_LVL= -O3
AVX_FLAGS= -mavx
CC_FLAGS = -std=c++17 -fno-elide-constructors -fno-prefetch-loop-arrays -fno-unroll-loops -ftree-vectorizer-verbose=1 -fno-gcse -fno-tree-pta -fno-tree-pre
CC = clang

CORE_ISOLATE = 1

RED=\033[0;31m
GREEN=\033[0;32m
NC=\033[0m

GRUB_REPLACE = 's/GRUB_CMDLINE_LINUX_DEFAULT="[a-z0-9 A-Z,\.=]*"/GRUB_CMDLINE_LINUX_DEFAULT="quiet splash isolcpus=${CORE_ISOLATE} nohz_full=${CORE_ISOLATE} rcu_nocbs=${CORE_ISOLATE} intel_idle.max_cstate=0 processor.max_cstate=0 idle=poll"/g'

.PHONY: all $(NAME) clean fclean re

all: $(NAME)

isolate_2nd_core:
	@echo -n "${RED}This will modify your grub settings to isolate 2nd core. Do this only if you are sure what are you doing. Old settings will be saved in /etc/default/grub.save. To undo 'make restore_grub'. ${GREEN}\nAre you sure [y/N] ${NC}" && read ans && [ $${ans:-N} = y ]
ifeq ("","$(wildcard /etc/default/grub.save)")
	sudo cp /etc/default/grub /etc/default/grub.save
else
	@echo -n "${RED}There is already saved configuration. Stop${NC}\n"
	@exit 1
endif
	sudo sed -i -e $(GRUB_REPLACE) /etc/default/grub
	sudo update-grub
	@echo -n "To apply changes your system have to be rebooted. Do you want to reboot now? [y/N] " && read ans && [ $${ans:-N} = y ]
	reboot

restore_grub:
ifneq ("$(wildcard /etc/default/grub.save)","")
	sudo mv /etc/default/grub.save /etc/default/grub
else
	@echo -n "${RED}No saved grub configuration${NC}"
endif
	sudo update-grub
	@echo -n "To apply changes your system have to be rebooted. Do you want to reboot now? [y/N] " && read ans && [ $${ans:-N} = y ]
	reboot

$(NAME):
ifeq ("","$(wildcard /etc/default/grub.save)")
	@echo -n "${GREEN}If you want to get more precise results run 'make isolate_2nd_core'.${NC}\n"
	@sleep 2
endif
	$(CC) $(CC_FLAGS) $(AVX_FLAGS) $(OPTIMISATION_LVL) -o $(NAME) $(FILES)
	taskset -c 1 ./$(NAME) --more-info

clean:
	rm $(NAME)

fclean: clean  restore_grub

