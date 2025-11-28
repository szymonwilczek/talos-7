import type { NextConfig } from "next";

const isElectron = process.env.NEXT_PUBLIC_IS_ELECTRON === "true";

const nextConfig: NextConfig = {
  output: isElectron ? "export" : undefined,
  images: {
    unoptimized: isElectron,
  },
};

export default nextConfig;
