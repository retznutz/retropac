<template>
    <header class="header">
        <a href="https://github.com/retznutz/retropac" target="_blank" rel="noopener">
            <img src="~/assets/logo.svg" alt="RetroPac" class="header-logo" />
        </a>
        <nav class="header-nav">
            <NuxtLink to="/" class="nav-link" :class="{ active: route.path === '/' }">
                <i class="bx bx-movie-play"></i> Animations
            </NuxtLink>
            <NuxtLink to="/config" class="nav-link" :class="{ active: route.path === '/config' }">
                <i class="bx bx-cog"></i> Config
            </NuxtLink>
        </nav>
        <div class="header-actions">
            <slot />
        </div>
    </header>
</template>

<script setup lang="ts">
const route = useRoute()
</script>
