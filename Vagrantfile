Vagrant.configure("2") do |config|
  config.vm.box = "puppetlabs/centos-6.6-64-puppet"
  config.vm.box_version = "1.0.1"

  config.vm.network :forwarded_port, guest: 80, host: 8080
  config.vm.network :private_network, ip: "192.168.7.7"

  config.vm.synced_folder '.', '/data', :nfs => false

  config.vm.hostname = "drupal.local"

  config.vm.provider :virtualbox do |v|
    v.name = "ctl-drupal"
    v.customize ["modifyvm", :id, "--cpus", "2"]
    v.customize ["modifyvm", :id, "--memory", "2048"]
  end

  config.vm.provision :puppet do |puppet|
    puppet.manifests_path = "puppet/manifests"
    puppet.manifest_file  = "site.pp"
    puppet.module_path    = "puppet/modules"
    puppet.options        = ["--verbose", "--hiera_config /data/puppet/hiera.yaml"]
  end
end
